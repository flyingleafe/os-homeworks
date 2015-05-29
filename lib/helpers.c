#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "helpers.h"

struct execargs_t {
    int in_fd;
    int out_fd;
    int pid;
    size_t argc;
    char* argv[];
};

execargs_t* get_execargs(size_t argc) {
    execargs_t* ea = (execargs_t*) malloc(sizeof(execargs_t) + (argc + 1) * sizeof(char*));

    if (ea == NULL) {
        return NULL;
    }

    ea->argc = argc;
    ea->argv[argc] = NULL;
    ea->pid = -1;
    ea->in_fd = -1;
    ea->out_fd = -1;
    return ea;
}

void free_execargs(execargs_t* ea) {
    free(ea);
}

char* get_arg(execargs_t* ea, int i) {
    return ea->argv[i];
}

void set_arg(execargs_t* ea, int i, char* arg) {
    ea->argv[i] = arg;
}

size_t get_argc(execargs_t* ea) {
    return ea->argc;
}

ssize_t read_(int fd, void* buf, size_t count) {
    int res = 0;
    int rres = 0;
    while(1) {
        res = read(fd, buf, count);

        if(res == count) {
            return rres + res;
        }

        if(res == -1) {
            return -1;
        }

        if(res == 0) {
            return rres;
        }

        rres += res;
        buf += res;
        count -= res;
    }
}

ssize_t write_(int fd, void* buf, size_t count) {
    int res = 0;
    int rres = 0;
    while(1) {
        res = write(fd, buf, count);

        if(res == count) {
            return rres + res;
        }

        if(res == -1) {
            return -1;
        }

        rres += res;
        buf += res;
        count -= res;
    }
}

ssize_t read_until(int fd, void* buf, size_t count, char delimeter) {
    int res = 0;
    int rres = 0;
    while(1) {
        res = read(fd, buf, count);

        int i;
        for(i = 0; i < res; i++) {
            if(((char*) buf)[i] == delimeter) {
                return rres + res;
            }
        }

        if(res == count) {
            return rres + res;
        }

        if(res == -1) {
           return -1;
        }

        if(res == 0) {
            return rres;
        }

        rres += res;
        buf += res;
        count -= res;
    }
}

void report_error() {
    char* err = strerror(errno);
    write_(STDERR_FILENO, err, strlen(err) * sizeof(char));
}

int spawn(const char * file, char * const argv []) {
    int pid = fork();

    RETHROW_IO(pid);

    if (pid == 0) {
        RETHROW_IO(execvp(file, argv));
    } else {
        int status;
        RETHROW_IO(wait(&status));

        if(!WIFEXITED(status)) {
            return -1;
        }

        return WEXITSTATUS(status);
    }

    // should never happen
    return -1;
}

int redirect_fd(int to_fd, int from_fd) {
    int storage = dup(from_fd);
    RETHROW_IO(storage);

    RETHROW_IO(dup2(to_fd, from_fd));
    return storage;
}

int suppress_output(int fd) {
    int dev_null = open("/dev/null", O_WRONLY);
    RETHROW_IO(dev_null);

    int storage = redirect_fd(dev_null, fd);
    RETHROW_IO(close(dev_null));
    return storage;
}

int restore_output(int fd, int storage) {
    RETHROW_IO(dup2(storage, fd));
    RETHROW_IO(close(storage));
    return 0;
}

int exec(execargs_t* args) {
    int pid = fork();

    if (pid == 0) {
        if (args->in_fd != -1) {
            RETHROW_IO(redirect_fd(args->in_fd, STDIN_FILENO));
            RETHROW_IO(close(args->in_fd));
        }
        if (args->out_fd != -1) {
            RETHROW_IO(redirect_fd(args->out_fd, STDOUT_FILENO));
            RETHROW_IO(close(args->out_fd));
        }
        int res = execvp(args->argv[0], args->argv);
        if (res == -1) {
            // Return errno to set it in parent
            _exit(errno);
        }
    }

    if (args->in_fd != -1) {
        RETHROW_IO(close(args->in_fd));
    }
    if (args->out_fd != -1) {
        RETHROW_IO(close(args->out_fd));
    }
    return args->pid = pid;
}

int unblock_signals(const sigset_t *smask) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    siginfo_t siginfo;
    int s = 0;
    while(1) {
        int sig = sigtimedwait(smask, &siginfo, &ts);
        if (sig == -1) {
            if (errno != EAGAIN) {
                s = -1;
            }
            break;
        }
    }

    RETHROW_IO(sigprocmask(SIG_UNBLOCK, smask, NULL));
    return s;
}

int runpiped(execargs_t** programs, size_t n) {

    sigset_t smask;
    sigemptyset(&smask);
    sigaddset(&smask, SIGINT);
    sigaddset(&smask, SIGCHLD);

    RETHROW_IO(sigprocmask(SIG_BLOCK, &smask, NULL));

    size_t i;
    for (i = 1; i < n; i++) {
        int s = pipe(&programs[i]->in_fd);
        if (s == -1) {
            // unblock signals
            RETHROW_IO(unblock_signals(&smask));

            // close all previously opened pipes
            size_t j;
            for (j = 1; j < i; j++) {
                RETHROW_IO(close(programs[i]->in_fd));
                RETHROW_IO(close(programs[i]->out_fd));
            }
            return -1;
        }
    }

    // reposition input/output file descriptors for a program
    for (i = 0; i < n - 1; i++) {
        programs[i]->out_fd = programs[i + 1]->out_fd;
    }
    // last program writes to stdout
    programs[n - 1]->out_fd = -1;

    // start child processes
    for (i = 0; i < n; i++) {
        int pid = exec(programs[i]);
        if (pid == -1) {
            // unblock signals
            RETHROW_IO(unblock_signals(&smask));

            // make sure all pipes are closed
            for (i = i + 1; i < n; i++) {
                RETHROW_IO(close(programs[i]->in_fd));
                RETHROW_IO(close(programs[i]->out_fd));
            }
            return -1;
        }
    }

    // wait for them
    siginfo_t siginfo;
    size_t finished = 0;
    int force_stop = 0;

    while (1) {
        int sig = sigwaitinfo(&smask, &siginfo);
        if (sig == -1) {
            unblock_signals(&smask);
            return -1;
        } else if (sig == SIGCHLD) {
            int status;
            // wait for all processes which may be ended
            while (finished < n) {
                int pid = waitpid(-1, &status, WNOHANG);

                if (pid == 0) {
                    break;
                }

                if (pid == -1) {
                    if (errno != ECHILD) {
                        force_stop = errno;
                    }
                    break;
                }

                // mark process as finished
                for (i = 0; i < n; i++) {
                    if (programs[i]->pid == pid) {
                        programs[i]->pid = -1;
                        break;
                    }
                }

                if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                    // if execve fails, we'll have errno in exit status
                    force_stop = WEXITSTATUS(status);
                    break;
                }

                // check if all processes are finished
                finished++;
            }

            if (finished == n || force_stop) {
                break;
            }
        } else if (sig == SIGINT) {
            force_stop = -1;
            break;
        }
    }

    if (force_stop) {
        // kill all children
        for (i = 0; i < n; i++) {
            if (programs[i]->pid != -1) {
                int s = kill(programs[i]->pid, SIGKILL);
                if (s == -1) {
                    unblock_signals(&smask);
                    return -1;
                }
            }
        }
    }

    // clear the pending signal queue
    int s = unblock_signals(&smask);
    if (force_stop > 0) {
        errno = force_stop;
        return -1;
    }
    return s;
}

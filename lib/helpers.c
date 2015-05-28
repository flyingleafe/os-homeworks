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
    RETHROW_IO(pid);

    if (pid == 0) {
        if (args->in_fd != -1) {
            RETHROW_IO(redirect_fd(args->in_fd, STDIN_FILENO));
        }
        if (args->out_fd != -1) {
            RETHROW_IO(redirect_fd(args->out_fd, STDOUT_FILENO));
        }
        RETHROW_IO(execvp(args->argv[0], args->argv));
    }

    if (args->in_fd != -1) {
        RETHROW_IO(close(args->in_fd));
    }
    if (args->out_fd != -1) {
        RETHROW_IO(close(args->out_fd));
    }
    args->pid = pid;
    return pid;
}

int pipe_parent_id;

void runpiped_sighandler(int sig) {
    printf("heeey some signal: %d\n", sig);
    if (pipe_parent_id != getpid()) {
        printf("child signal!\n");
        _exit(0);
    } else if (sig == SIGINT) {
        printf("parent signal!\n");
        kill(-pipe_parent_id, SIGQUIT);
    }
}

int runpiped(execargs_t** programs, size_t n) {

    pipe_parent_id = getpid();
    signal(SIGINT, runpiped_sighandler);

    size_t i;
    for (i = 1; i < n; i++) {
        RETHROW_IO(pipe(&programs[i]->in_fd));
    }

    // reposition input/output file descriptors for a program
    for (i = 0; i < n - 1; i++) {
        programs[i]->out_fd = programs[i + 1]->out_fd;
    }
    // last program writes to stdout
    programs[n - 1]->out_fd = -1;

    // start child processes
    for (i = 0; i < n; i++) {
        RETHROW_IO(exec(programs[i]));
    }

    // wait for them
    int status;
    while (1) {
        int child = wait(&status);
        if (child == -1 && errno == ECHILD) {
            // no more children left
            break;
        } else if (child > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            for (i = 0; i < n; i++) {
                if (programs[i]->pid != child) {
                    kill(programs[i]->pid, SIGINT);
                }
            }
            break;
        } else if (child == -1 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }


    return 0;
}

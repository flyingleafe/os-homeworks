#include <helpers.h>
#include <bufio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

char s[5000];
char s2[5000];

buf_t* buf;
buf_t* buf2;

int suppress_output(int fd) {
    int dev_null = open("/dev/null", O_WRONLY);
    RETHROW_IO(dev_null);

    int storage = dup(fd);
    RETHROW_IO(storage);

    RETHROW_IO(dup2(dev_null, fd));
    RETHROW_IO(close(dev_null));
    return storage;
}

int restore_output(int fd, int storage) {
    RETHROW_IO(dup2(storage, fd));
    RETHROW_IO(close(storage));
    return 0;
}

int process_string(char * src, size_t len, int argc, char * pargs[]) {
    memcpy(s2, src, len * sizeof(char));
    s2[len] = '\0';

    pargs[argc - 1] = s2;

    int res = 0;

    int stdout_copy = suppress_output(STDOUT_FILENO);
    RETHROW_IO(stdout_copy);
    int stderr_copy = suppress_output(STDERR_FILENO);
    RETHROW_IO(stderr_copy);

    res = spawn(pargs[0], pargs);

    RETHROW_IO(restore_output(STDOUT_FILENO, stdout_copy));
    RETHROW_IO(restore_output(STDERR_FILENO, stderr_copy));
    RETHROW_IO(res);

    if(res == 0) {
        s2[len] = '\n';
        RETHROW_IO(buf_write(STDOUT_FILENO, buf2, s2, len + 1));
    }
    return 0;
}

int main(int argc, char * argv []) {
    if (argc < 2) {
        write_(STDIN_FILENO, "Usage:\n    ./buffilter <program_name> [program_args ...]\n", 57);
        return 0;
    }

    char** pargs = (char **) calloc(argc + 1, sizeof(char *));
    memcpy(pargs, argv + 1, (argc - 1) * sizeof(char *));
    pargs[argc] = NULL;

    int rres = 0;
    ssize_t len = 0;

    buf = buf_new(4096);
    buf2 = buf_new(4096);

    while (1) {
        len = buf_getline(STDIN_FILENO, buf, s);

        if (len == 0 || len == -1) {
            break;
        }

        rres = process_string(s, len, argc, pargs);

        if (rres == -1) {
            break;
        }
    }

    buf_flush(STDOUT_FILENO, buf2, buf_size(buf2));

    buf_free(buf);
    buf_free(buf2);
    free(pargs);
    CATCH_IO(rres);
    CATCH_IO(len);

    return 0;
}

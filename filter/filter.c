#include <helpers.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

char buf[5000];
char buf2[5000];

int suppress_output(int fd) {
    int dev_null = open("/dev/null", O_WRONLY);
    RETHROW_IO(dev_null);

    int storage = dup(STDOUT_FILENO);
    RETHROW_IO(storage);

    RETHROW_IO(dup2(dev_null, STDOUT_FILENO));
    return storage;
}

int restore_output(int fd, int storage) {
    RETHROW_IO(dup2(storage, fd));
    RETHROW_IO(close(storage));
    return 0;
}

int process_string(char * src, size_t len, int argc, char * pargs[]) {
    memcpy(buf2, src, len * sizeof(char));
    buf2[len] = '\0';

    pargs[argc - 1] = buf2;

    int res = 0;

    int stdout_copy = suppress_output(STDOUT_FILENO);
    RETHROW_IO(stdout_copy);
    int stderr_copy = suppress_output(STDERR_FILENO);
    RETHROW_IO(stderr_copy);

    res = spawn(pargs[0], pargs);

    RETHROW_IO(restore_output(STDOUT_FILENO, stdout_copy));
    RETHROW_IO(restore_output(STDERR_FILENO, stderr_copy));

    if(res == 0) {
        buf2[len] = '\n';
        RETHROW_IO(write_(STDOUT_FILENO, buf2, (len + 1) * sizeof(char)));
    }
    return 0;
}

int main(int argc, char * argv []) {
    if (argc < 2) {
        write_(STDIN_FILENO, "Usage:\n    ./filter <program_name> [program_args ...]\n", 54);
        return 0;
    }

    char** pargs = (char **) calloc(argc, sizeof(char *));
    memcpy(pargs, argv + 1, (argc - 1) * sizeof(char *));

    int rres = 0;
    int wres = 0;
    int i = 0;
    int j = 0;

    while (1) {
        ssize_t offset = rres;
        rres = read_until(STDIN_FILENO, buf + offset * sizeof(char), sizeof(buf) - offset * sizeof(char), '\n');

        // we reached EOF or encountered an error: write the rest of buffer and halt.
        if (rres == 0 || rres == -1) {

            wres = process_string(buf, offset, argc, pargs);

            CATCH_IO(wres);
            CATCH_IO(rres);

            if (rres == 0) {
                return 0;
            }
        }

        rres += offset;

        // write all the lines in buffer
        offset = 0;
        char has_delim = 0;
        for (i = 0; i < rres; i++) {
            if(buf[i] == '\n') {
                wres = process_string(buf + offset * sizeof(char), i - offset, argc, pargs);

                CATCH_IO(wres);

                offset = i + 1;
                has_delim = 1;
            }
        }

        // if we had space and written something,
        // copy the rest of buffer in the beginning
        if (has_delim) {
            rres -= offset * sizeof(char);
            if(rres > 0) {
                memmove(buf, buf + offset * sizeof(char), rres);
            }
        }
    }

    free(pargs);

    return 0;
}

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <helpers.h>
#include <errno.h>
#include <stdio.h>

execargs_t* parse_program(char* input) {
    size_t argc = 0;
    char* input2 = input;

    // count argc
    while(*input2) {
        input2 += strspn(input2, " ");
        size_t wlen = strcspn(input2, " \n");
        if (wlen == 0) {
            break;
        }
        input2 += wlen;
        argc++;
    }

    // return NULL if error, not consuming the input
    if (argc == 0) {
        return NULL;
    }

    execargs_t* args = get_execargs(argc);

    // read argv
    int i = 0;
    char* save;
    char* word = strtok_r(input, " \n", &save);

    while (word != NULL) {
        set_arg(args, i, word);
        i++;
        word = strtok_r(NULL, " \n", &save);
    }

    return args;
}

int parse_sh(char* input, execargs_t** programs) {
    int i = 0;

    char* saveptr;
    char* prog = strtok_r(input, "|\n", &saveptr);
    while (prog != NULL) {
        execargs_t* ea = parse_program(prog);

        if (ea == NULL) {
            break;
        }
        programs[i] = ea;

        i++;
        prog = strtok_r(NULL, "|\n", &saveptr);
    }

    return i;
}

int main(int argc, char *argv[]) {

    // ignore the SIGINT
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);

    CATCH_IO(sigaction(SIGINT, &sa, NULL));

    char buf[4096];
    execargs_t* programs[2048];

    int rres;
    while (1) {
        // print the prompt
        CATCH_IO(write_(STDOUT_FILENO, "$", 1));

        rres = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        CATCH_IO(rres);

        if (rres == 0) {
            break;
        }

        buf[rres] = '\0';

        int n = parse_sh(buf, programs);
        if (n > 0) {
            int res = runpiped(programs, n);

            if (res == -1) {
                perror("Error");
            }

            int i
            for (i = 0; i < n; i++) {
                free_execargs(programs[i]);
            }
        }
    }

    return 0;
}

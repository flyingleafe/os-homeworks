#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <helpers.h>
#include <errno.h>
#include <stdio.h>

void skip_whitespace(char** input) {
    while((*input)[0] == ' ') {
        (*input)++;
    }
}

size_t parse_word(char* input) {
    size_t len = 0;
    while(input[len] != '|' && input[len] != ' ' && input[len] != '\n' && input[len]) {
        len++;
    }
    return len;
}

execargs_t* parse_program(char** input) {
    size_t argc = 0;
    char* input2 = *input;

    // count argc
    while(*input2) {
        size_t wlen = parse_word(input2);
        if (wlen == 0 || input2[0] == '|') {
            break;
        }

        input2 += wlen;
        skip_whitespace(&input2);
        argc++;
    }


    // return NULL if error, not consuming the input
    if (argc == 0) {
        return NULL;
    }

    execargs_t* args = get_execargs(argc);

    // read argv
    int i;
    for (i = 0; i < argc; i++) {
        size_t wlen = parse_word(*input);
        set_arg(args, i, strndup(*input, wlen));
        *input += wlen;
        skip_whitespace(input);
    }

    if ((*input)[0] == '|') {
        (*input)++;
        skip_whitespace(input);
    }
    return args;
}

int parse_sh(char* input, execargs_t** programs) {
    int i = 0;

    while (*input) {
        execargs_t* cur = parse_program(&input);
        if (cur == NULL) {
            break;
        }
        programs[i] = cur;
        i++;
    }

    return i;
}

int main(int argc, char *argv[]) {

    signal(SIGINT, SIG_IGN);
    char buf[4096];
    execargs_t* programs[4096];

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
                perror("something bad happened:");
                perror(strerror(errno));
            }

            int i, j;
            for (i = 0; i < n; i++) {
                for (j = 0; j < get_argc(programs[i]); j++) {
                    free(get_arg(programs[i], j));
                }
                free_execargs(programs[i]);
            }
        }
    }

    return 0;
}

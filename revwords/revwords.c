#include <unistd.h>
#include <stdlib.h>
#include <helpers.h>
#include <string.h>

#define CATCH_IO(res) if (res == -1) {          \
        report_error();                         \
        return 1;                               \
    }

void reverse(char* buf, int len) {
    int i;
    char t;
    for(i = 0; i < len/2; i++) {
        t = buf[i];
        buf[i] = buf[len - i - 1];
        buf[len - i - 1] = t;
    }
}

int main(int argc, char* argv[]) {
    char buf[5000];
    ssize_t rres = 0;
    ssize_t wres = 0;
    int i = 0;
    int j = 0;

    while(1) {
        ssize_t offset = rres;
        rres = read_until(STDIN_FILENO, buf + offset * sizeof(char), sizeof(buf) - offset * sizeof(char), ' ');

        // we reached EOF or encountered an error: write the rest of buffer and halt.
        if (rres == 0 || rres == -1) {
            reverse(buf, offset);
            wres = write_(STDOUT_FILENO, buf, offset * sizeof(char));

            CATCH_IO(wres);
            CATCH_IO(rres);

            if (rres == 0) {
                return 0;
            }
        }

        rres += offset;

        // write all the words in buffer
        offset = 0;
        char has_space = 0;
        for (i = 0; i < rres; i++) {
            if(buf[i] == ' ') {
                reverse(buf + offset * sizeof(char), i - offset);
                wres = write_(STDOUT_FILENO, buf + offset * sizeof(char), (i - offset + 1) * sizeof(char));

                CATCH_IO(wres);

                offset = i;
                has_space = 1;
            }
        }

        // if we had space and written something,
        // copy the rest of buffer in the beginning
        if (has_space) {
            rres -= (offset + 1) * sizeof(char);
            if(rres > 0) {
                memmove(buf, buf + (offset + 1) * sizeof(char), rres);
            }
        }
    }
}

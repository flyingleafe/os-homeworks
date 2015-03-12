#include <unistd.h>
#include <stdlib.h>
#include <helpers.h>
#include <string.h>

#define CATCH_IO(res) if (res == -1) {\
    report_error();\
    return 1;\
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
    char buf[4097];
    ssize_t rres = 0;
    ssize_t wres = 0;
    int i = 0;
    int j = 0;

    while(1) {
        ssize_t offset = rres * sizeof(char);
        rres = read_until(STDIN_FILENO, buf + offset, sizeof(buf) - offset, ' ');

        if (rres == 0 || rres == -1) {
            reverse(buf, offset / sizeof(char));
            wres = write_(STDOUT_FILENO, buf, offset);

            CATCH_IO(wres);
            CATCH_IO(rres);

            if (rres == 0) {
                return 0;
            }
        }

        rres += offset;

        while(1) {
            for (i = 0; i < rres; i++) {
                if(buf[i] == ' ') {
                    break;
                }
            }

            if (i == rres) {
                break;
            }

            reverse(buf, i);
            wres = write_(STDOUT_FILENO, buf, (i + 1) * sizeof(char));

            CATCH_IO(wres);

            rres -= i + 1;
            if(rres == 0) {
                break;
            }
            memmove(buf, buf + (i + 1) * sizeof(char), rres * sizeof(char));
        }
    }
}

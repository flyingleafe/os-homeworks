#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <helpers.h>

void report_error() {
    char* err = strerror(errno);
    write_(STDERR_FILENO, err, strlen(err) * sizeof(char));
}

int main(int argc, char* argv[]) {
    char buf[4096];
    ssize_t rres = 0;
    ssize_t wres = 0;

    while(1) {
        rres = read_(STDIN_FILENO, buf, sizeof(buf));
        if(rres == -1) {
            report_error();
            return 1;
        }

        wres = write_(STDOUT_FILENO, buf, rres);
        if(wres == -1) {
            report_error();
            return 1;
        }
        
        if(rres < sizeof(buf)) {
            return 0;
        }
    }
}

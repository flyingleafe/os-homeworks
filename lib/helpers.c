#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "helpers.h"

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

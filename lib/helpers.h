#ifndef FLF_OS_HELPERS
#define FLF_OS_HELPERS

#include <unistd.h>

#define CATCH_IO(res) if ((res) == -1) {        \
        report_error();                         \
        return 1;                               \
    }

#define RETHROW_IO(res) if ((res) == -1) {      \
        return -1;                              \
    }


ssize_t read_(int fd, void* buf, size_t count);
ssize_t write_(int fd, void* buf, size_t count);
ssize_t read_until(int fd, void* buf, size_t count, char delimiter);
int spawn(const char * file, char * const argv []);
void report_error();

#endif

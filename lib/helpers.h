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

struct execargs_t;
typedef struct execargs_t execargs_t;

execargs_t* get_execargs(size_t argc);
void free_execargs(execargs_t* ea);
char* get_arg(execargs_t* ea, int i);
void set_arg(execargs_t* ea, int i, char* arg);
size_t get_argc(execargs_t* ea);

ssize_t read_(int fd, void* buf, size_t count);
ssize_t write_(int fd, void* buf, size_t count);
ssize_t read_until(int fd, void* buf, size_t count, char delimiter);
int spawn(const char * file, char * const argv []);
int exec(execargs_t* args);
int runpiped(execargs_t** programs, size_t n);
void report_error();

#endif

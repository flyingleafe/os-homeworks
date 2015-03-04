#ifndef FLF_OS_HELPERS
#define FLF_OS_HELPERS
// #include <sys/types.h>

ssize_t read_(int fd, void* buf, size_t count);
ssize_t write_(int fd, void* buf, size_t count);

#endif

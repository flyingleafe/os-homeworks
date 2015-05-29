#ifndef FLF_OS_BUFIO
#define FLF_OS_BUFIO

struct buf_t;
typedef struct buf_t buf_t;
typedef int fd_t;

buf_t *buf_new(size_t capacity);
void buf_free(struct buf_t *);
size_t buf_capacity(buf_t *);
size_t buf_size(buf_t *);
ssize_t buf_fill(fd_t fd, buf_t *buf, size_t required);
ssize_t buf_flush(fd_t fd, buf_t *buf, size_t required);
ssize_t buf_getline(fd_t fd, buf_t *buf, char *dest);
ssize_t buf_write(fd_t fd, buf_t *buf, char *src, size_t len);

#endif

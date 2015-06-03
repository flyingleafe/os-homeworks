#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bufio.h"
#include "helpers.h"

#ifdef DEBUG

#define ABORT_IF(cond) if (cond) {abort();}

#else

#define ABORT_IF(cond)

#endif

struct buf_t {
    size_t capacity;
    size_t size;

    void* data;
};

buf_t *buf_new(size_t capacity) {
    buf_t* newbuf = (buf_t*) malloc(sizeof(buf_t) + capacity);

    if (newbuf == NULL) {
        return NULL;
    }

    newbuf->data = (void*) newbuf + sizeof(buf_t);
    newbuf->capacity = capacity;
    newbuf->size = 0;
    return newbuf;
}

void buf_free(buf_t *buf) {
    ABORT_IF(buf == NULL);
    free(buf);
}

size_t buf_capacity(buf_t *buf) {
    ABORT_IF(buf == NULL);
    return buf->capacity;
}

size_t buf_size(buf_t *buf) {
    ABORT_IF(buf == NULL);
    return buf->size;
}

ssize_t buf_fill(fd_t fd, buf_t *buf, size_t required) {
    ABORT_IF(buf == NULL);
    ABORT_IF(required > buf->capacity);

    int res = 0;
    int rest = required - buf->size;
    while(1) {
        res = read(fd, buf->data + buf->size, buf->capacity - buf->size);
        RETHROW_IO(res);

        if(res >= rest) {
            return buf->size += res;
        }

        if(res == 0) {
            return buf->size;
        }

        buf->size += res;
        rest -= res;
    }
}

ssize_t buf_flush(fd_t fd, buf_t *buf, size_t required) {
    ABORT_IF(buf == NULL);
    ABORT_IF(required > buf->capacity);

    int res = 0;
    int rest = required;
    int written = 0;
    while(1) {
        res = write(fd, buf->data + written, buf->size - written);

        if(res == -1) {
            break;
        }

        if(res >= rest) {
            written += res;
            break;
        }

        written += res;

        if(written == buf->size) {
            break;
        }

        rest -= res;
    }

    buf->size -= written;
    if (buf->size > 0) {
        memmove(buf, buf + written, buf->size);
    }

    RETHROW_IO(res);
    return written;
}

ssize_t buf_getline(fd_t fd, buf_t *buf, char *dest) {
    ABORT_IF(buf == NULL);
    int len = 0;

    while(1) {
        for (int i = 0; i < buf->size; i++) {
            if (((char*)buf->data)[i] == '\n') {
                memcpy(dest, buf->data, i);
                buf->size -= i + 1;

                if (buf->size > 0) {
                    memmove(buf->data, buf->data + i + 1, buf->size);
                }

                return len + i;
            }
        }

        if (buf->size > 0) {
            memcpy(dest, buf->data, buf->size);
            dest += buf->size;
            len += buf->size;
            buf->size = 0;
        }

        int res = buf_fill(fd, buf, 1);
        RETHROW_IO(res);

        // got EOF
        if (res < 1) {
            return len;
        }
    }
}

ssize_t buf_write(fd_t fd, buf_t *buf, char *src, size_t len) {
    int written = 0;

    while(len > buf->capacity - buf->size) {
        size_t rest = buf->capacity - buf->size;
        int l = rest < len ? rest : len;

        memcpy(buf->data + buf->size, src, l);
        buf->size += l;
        src += l;
        len -= l;
        int wr = buf_flush(fd, buf, buf->size);
        RETHROW_IO(wr);
        written += wr;
    }

    if(len > 0) {
        memcpy(buf->data + buf->size, src, len);
        buf->size += len;
        written += len;
    }

    return written;
}

int cat(fd_t input, fd_t output) {
    buf_t* buf = buf_new(4096);
    if (buf == NULL) {
        return -1;
    }

    ssize_t res = 1;
    while(res) {
        if (buf_fill(input, buf, 1) < 0) {
            buf_free(buf);
            return -1;
        }
        res = buf_flush(output, buf, buf_size(buf));
        if (res < 0) {
            buf_free(buf);
            return -1;
        }
    }

    buf_free(buf);
    return 0;
}

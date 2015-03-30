#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
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
    buf_t* newbuf = (buf_t*) malloc(2 * sizeof(size_t) + sizeof(void *) + capacity);

    if (newbuf == NULL) {
        return NULL;
    }

    newbuf->capacity = capacity;
    newbuf->size = 0;
    newbuf->data = &(newbuf->data) + sizeof(void *);
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

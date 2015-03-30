#include <unistd.h>
#include <helpers.h>
#include <bufio.h>

int main(int argc, char *argv []) {
    buf_t* buf = buf_new(4096);
    size_t cap = buf_capacity(buf);
    ssize_t read_res = 0;
    ssize_t write_res = 0;
    while(1) {
        read_res = buf_fill(STDIN_FILENO, buf, cap);
        write_res = buf_flush(STDOUT_FILENO, buf, buf_size(buf));
        CATCH_IO(read_res);
        CATCH_IO(write_res);
        if(read_res < cap) {
            return 0;
        }
    }
    buf_free(buf);
}

#define _GNU_SOURCE
#include <helpers.h>
#include <bufio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <poll.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    buf_t* in;
    buf_t* out;
} pipebuf;

pipebuf pipebuf_new(size_t cap) {
    pipebuf res;
    res.in = buf_new(cap);
    res.out = buf_new(cap);
    return res;
}

void pipebuf_free(pipebuf *pb) {
    buf_free(pb->in);
    buf_free(pb->out);
}

struct pollfd pollfds[256];
pipebuf buffs[127];
char valid_in[256];
char valid_out[256];
int count = 2;

// Dual index - +1 if even, -1 if odd
int dual(int i) {
    return i + 1 - (i & 1) * 2;
}

// Lead index - round by 2
int lead(int i) {
    return i & ~1;
}

void add_pair(int fd1, int fd2) {

    pollfds[count].fd = fd1;
    pollfds[count + 1].fd = fd2;
    pollfds[count].events = pollfds[count + 1].events = POLLIN;
    pollfds[count].revents = pollfds[count + 1].revents = 0;

    buffs[(count - 2) / 2] = pipebuf_new(4096);
    valid_in[count] = valid_in[count + 1] = 1;
    valid_out[count] = valid_out[count + 1] = 1;

    count += 2;

    // stop ignoring both acceptfds
    pollfds[0].fd = abs(pollfds[0].fd);
    pollfds[1].fd = abs(pollfds[1].fd);
}

int remove_pair(int index) {
    index = lead(index);
    count -= 2;

    if (count != index) {
        // Swap fds
        struct pollfd tmp = pollfds[index];
        pollfds[index] = pollfds[count];
        pollfds[count] = tmp;
        tmp = pollfds[index + 1];
        pollfds[index + 1] = pollfds[count + 1];
        pollfds[count + 1] = tmp;

        // Swap valids
        char tmpf = valid_in[index];
        valid_in[index] = valid_in[count];
        valid_in[count] = tmpf;
        tmpf = valid_out[index];
        valid_out[index] = valid_out[count];
        valid_out[count] = tmpf;

        // Swap buffs
        pipebuf tmpb = buffs[(index - 2) / 2];
        buffs[(index - 2) / 2] = buffs[(count - 2) / 2];
        buffs[(count - 2) / 2] = tmpb;
    }

    pipebuf_free(&buffs[(count - 2) / 2]);
    RETHROW_IO(close(abs(pollfds[count].fd)));
    RETHROW_IO(close(abs(pollfds[count + 1].fd)));
    return 0;
}

int accept_from(int i, int* remote1, int* remote2) {
    // something terrible happened
    if (pollfds[i].revents != POLLIN) {
        return -1;
    }
    *remote1 = accept4(pollfds[i].fd, NULL, NULL, SOCK_NONBLOCK);
    if (*remote1 < 0) {
        if (errno != EWOULDBLOCK) {
            perror("accept");
            return -1;
        }
    } else {
        if (*remote2 > 0) {
            add_pair(*remote1, *remote2);
            *remote1 = *remote2 = -1;
        } else {
            // ignore other accepts for a while okay
            pollfds[i].fd = -pollfds[i].fd;
        }
    }
}

int read_sock(int i) {
    buf_t* buf;
    if (i % 2 == 0) {
        buf = buffs[(i - 2) / 2].in;
    } else {
        buf = buffs[(i - 2) / 2].out;
    }

    // Remote end has stopped - we have nothing to do here
    if (pollfds[dual(i)].fd < 0) {
        RETHROW_IO(remove_pair(i));
        return 0;
    }

    size_t prev_size = buf_size(buf);
    int res = buf_fill(pollfds[i].fd, buf, 1);
    RETHROW_IO(res);

    // EOF
    if (res == prev_size) {
        // stop pollin
        pollfds[i].events &= ~POLLIN;
        valid_in[i] = 0;
        
        int s = shutdown(pollfds[i].fd, SHUT_RD);
        if (s == -1 && errno != ENOTCONN) {
            return -1;
        }

        if (!valid_out[i] || !valid_in[dual(i)]) {
            RETHROW_IO(remove_pair(i));
        }

        return 0;
    }

    // allow pollout for sink
    if (valid_out[dual(i)]) {
        pollfds[dual(i)].events |= POLLOUT;
    }

    // restrict pollin for source
    if (res == buf_capacity(buf)) {
        pollfds[i].events &= ~POLLIN;
    }
    return res - prev_size;
}

int write_sock(int i) {
    buf_t* buf;
    if (i % 2 == 0) {
        buf = buffs[(i - 2) / 2].out;
    } else {
        buf = buffs[(i - 2) / 2].in;
    }

    int res = buf_flush(pollfds[i].fd, buf, buf_size(buf));
    RETHROW_IO(res);

    // Error occured
    if (res == 0) {
        pollfds[i].events &= ~POLLOUT;
        valid_out[i] = 0;
        
        int s = shutdown(pollfds[i].fd, SHUT_WR);
        if (s == -1 && errno != ENOTCONN) {
            return -1;
        }

        if (!valid_in[i] || !valid_out[dual(i)]) {
            RETHROW_IO(remove_pair(i));
        }

        return 0;
    }

    // allow pollin for source
    if (valid_in[dual(i)]) {
        pollfds[dual(i)].events |= POLLIN;
    }

    // restrict pollout for sink
    if (buf_size(buf) == 0) {
        pollfds[i].events &= ~POLLOUT;
    }
    return res;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        write_(STDOUT_FILENO, "Usage:\n    forking port1 port2\n", 33);
        return 0;
    }

    int sock1, sock2;
    CATCH_IO(sock1 = make_server_socket(argv[1]));
    CATCH_IO(sock2 = make_server_socket(argv[2]));
    CATCH_IO(fcntl(sock1, F_SETFL, O_NONBLOCK));
    CATCH_IO(fcntl(sock2, F_SETFL, O_NONBLOCK));
    CATCH_IO(listen(sock1, 1));
    CATCH_IO(listen(sock2, 1));

    memset(valid_in, 1, sizeof(valid_in));
    memset(valid_out, 1, sizeof(valid_out));

    pollfds[0].fd = sock1;
    pollfds[1].fd = sock2;
    pollfds[0].events = pollfds[1].events = POLLIN;

    int remote1 = -1;
    int remote2 = -1;

    while (1) {
        CATCH_IO(poll(pollfds, count, -1));
        if (pollfds[0].revents > 0) {
            CATCH_IO(accept_from(0, &remote1, &remote2));
        }
        if (pollfds[1].revents > 0) {
            CATCH_IO(accept_from(1, &remote2, &remote1));
        }

        int i;
        for (i = 2; i < count; i++) {
            if (pollfds[i].revents != 0) {
                if (pollfds[i].revents & POLLIN) {
                    CATCH_IO(read_sock(i));
                }
                if (pollfds[i].revents & POLLOUT) {
                    CATCH_IO(write_sock(i));
                }
                if (pollfds[i].revents & POLLHUP) {
                    // read the rest of data
                    int rs = 1;
                    while (rs) {
                        CATCH_IO(rs = read_sock(i));
                    }
                    // tell other end we don't want to read from it anymore
                    rs = shutdown(pollfds[dual(i)].fd, SHUT_RD);
                    if (rs == -1 && errno != ENOTCONN) {
                        perror("shutdown");
                        return 1;
                    }
                    // and then just ignore it
                    valid_in[i] = 0;
                    valid_out[i] = 0;
                    pollfds[i].fd = -pollfds[i].fd;
                }
            }
        }
    }

    CATCH_IO(close(sock1));
    CATCH_IO(close(sock2));
    return 0;
}

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
#include <string.h>
#include <math.h>

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
int count = 2;

void add_pair(int fd1, int fd2) {
    assert(count <= 254);

    pollfds[count].fd = fd1;
    pollfds[count + 1].fd = fd2;
    pollfds[count].events = pollfds[count + 1].events = POLLIN;

    buffs[(count - 2) / 2] = pipebuf_new(4096);
    count += 2;

    // stop ignoring both ports
    pollfds[0].fd = abs(pollfds[0].fd);
    pollfds[1].fd = abs(pollfds[1].fd);
}

void remove_pair(int index) {
    assert(index % 2 == 0);

    pipebuf_free(&buffs[index / 2]);
    memmove(pollfds + index, pollfds + index + 2, count - index - 2);

    count -= 2;
}

int accept_from(int i, int* remote1, int* remote2) {
    // something terrible happened
    if (pollfds[i].revents != POLLIN) {
        return 1;
    }
    *remote1 = accept4(pollfds[i].fd, NULL, NULL, SOCK_NONBLOCK);
    if (*remote1 < 0) {
        if (errno != EWOULDBLOCK) {
            perror("accept");
            return 1;
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


int main(int argc, char *argv[])
{
    if (argc < 3) {
        write_(STDOUT_FILENO, "Usage:\n    forking port1 port2\n", 33);
        return 0;
    }

    int sock1, sock2;
    CATCH_IO(sock1 = make_server_socket(argv[1]));
    CATCH_IO(sock2 = make_server_socket(argv[2]));
    CATCH_IO(fcntl(sock1, F_SETFL, O_NONBLOCk));
    CATCH_IO(fcntl(sock2, F_SETFL, O_NONBLOCK));
    CATCH_IO(listen(sock1, 1));
    CATCH_IO(listen(sock2, 1));

    pollfds[0].fd = sock1;
    pollfds[1].fd = sock2;
    pollfds[0].events = pollfds[1].events = POLLIN;

    int remote1 = -1;
    int remote2 = -1;

    while (1) {
        CATCH_IO(poll(pollfds, count, -1));
        if (pollfds[0].revents > 0) {
            accept_from(0, &remote1, &remote2);
        }
        if (pollfds[1].revents > 0) {
            accept_from(1, &remote2, &remote1);
        }

        int i;
        for (i = 2; i < count; i++) {
            if (pollfds[i].revents > 0) {
                if (pollfds[i].revents & POLLIN != 0) {

                }
            }
        }
    }

    CATCH_IO(close(sock1));
    CATCH_IO(close(sock2));
    return 0;
}

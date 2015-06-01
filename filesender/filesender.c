#include <helpers.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <strings.h>
#include <bufio.h>

int make_socket_from_addrinfo(struct addrinfo* info) {
    while (info) {
        int sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock == -1) continue;

        int dummy = 1;
        int s = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(dummy));
        if (s == -1) {
            RETHROW_IO(close(sock));
            continue;
        }

        s = bind(sock, info->ai_addr, info->ai_addrlen);
        if (s == 0) {
            return sock;
        }

        RETHROW_IO(close(sock));
        info = info->ai_next;
    }

    return -1;
}

int make_server_socket(const char* port) {
    struct addrinfo *info, hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(NULL, port, &hints, &info);
    if (s != 0) {
        perror(gai_strerror(s));
        return -1;
    }

    int sock = make_socket_from_addrinfo(info);
    freeaddrinfo(info);
    return sock;
}

int cat(int input, int output) {
    buf_t* buf = buf_new(4096);
    if (buf == NULL) {
        return -1;
    }

    ssize_t res = 1;
    while(res) {
        RETHROW_IO(buf_fill(input, buf, 1));
        RETHROW_IO(res = buf_flush(output, buf, buf_size(buf)));
    }

    buf_free(buf);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        write_(STDOUT_FILENO, "Usage:\n    filesender port file\n", 32);
        return 0;
    }

    CATCH_IO(access(argv[2], R_OK | F_OK));

    int sock;
    CATCH_IO(sock = make_server_socket(argv[1]));
    CATCH_IO(listen(sock, 1));

    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    while(1) {
        int remote, pid;
        CATCH_IO(remote = accept(sock, (struct sockaddr*) &client, &len));
        CATCH_IO(pid = fork());

        if (pid == 0) {
            int file;

            CATCH_IO(file = open(argv[2], O_RDONLY));
            int s = cat(file, remote);
            int exit_status = 0;

            if (s == -1) {
                perror("Error while sending file");
                exit_status = 1;
            }

            close(remote);
            close(file);
            _exit(0);
        }

        CATCH_IO(close(remote));

        int status;
        while (pid > 0) {
            pid = waitpid(-1, &status, WNOHANG);
        }
    }

    CATCH_IO(close(sock));
    return 0;
}

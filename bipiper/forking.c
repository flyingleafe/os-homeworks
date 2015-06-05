#include <helpers.h>
#include <bufio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <signal.h>

void redirect_data(fd_t from, fd_t to) {
    int exit_status = 0;

    int s = cat(from, to);
    if (s == -1) {
        perror("Error while sending data");
        exit_status = 1;
    }

    close(from);
    close(to);
    _exit(exit_status);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        write_(STDOUT_FILENO, "Usage:\n    forking port1 port2\n", 33);
        return 0;
    }

    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_RESTART;
    CATCH_IO(sigaction(SIGCHLD, &sa, NULL));
 
    int sock1, sock2;
    CATCH_IO(sock1 = make_server_socket(argv[1]));
    CATCH_IO(sock2 = make_server_socket(argv[2]));
    CATCH_IO(listen(sock1, 1));
    CATCH_IO(listen(sock2, 1));

    struct sockaddr_in client1, client2;
    socklen_t len1 = sizeof(client1), len2 = sizeof(client2);

    while (1) {
        int remote1, remote2;
        CATCH_IO(remote1 = accept(sock1, (struct sockaddr*) &client1, &len1));
        CATCH_IO(remote2 = accept(sock2, (struct sockaddr*) &client2, &len2));

        int pid;
        CATCH_IO(pid = fork());

        if (pid == 0) {
            redirect_data(remote1, remote2);
        }

        CATCH_IO(pid = fork());

        if (pid == 0) {
            redirect_data(remote2, remote1);
        }

        CATCH_IO(close(remote1));
        CATCH_IO(close(remote2));
    }

    CATCH_IO(close(sock1));
    CATCH_IO(close(sock2));
    return 0;
}

#include <helpers.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <signal.h>
#include <bufio.h>

int main(int argc, char *argv[])
{
    if (argc < 3) {
        write_(STDOUT_FILENO, "Usage:\n    filesender port file\n", 32);
        return 0;
    }

    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_RESTART;
    CATCH_IO(sigaction(SIGCHLD, &sa, NULL));
    
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
            _exit(exit_status);
        }

        CATCH_IO(close(remote));
    }

    CATCH_IO(close(sock));
    return 0;
}

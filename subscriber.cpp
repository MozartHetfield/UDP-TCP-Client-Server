#include "sun_lib.h"

using namespace std;

int main(int argc, char *argv[]) {

    struct sockaddr_in serv_addr;
    fd_set read_fds, tmp_fds;
	int fdmax, sockfd;
	char buffer[BUFLEN], helper[BUFLEN];
    int ret, n;

	DIE(argc != 4, "Usage: ./subscriber <id_client> <ip_server> <port_server>");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));

	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton sin_addr ip");

    ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
    FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

    // send a message containing subscriber's ID
    memset(buffer, 0, BUFLEN);
    sprintf(buffer, "%d %d ", atoi(argv[3]), atoi(argv[1]));
    ret = send(sockfd, buffer, strlen(buffer), 0);
    DIE(ret < 0, "send ID");

    while (1) {
        tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == STDIN_FILENO) { // received input from subscriber
                    memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
                    if (strcmp(buffer, EXIT) == 0) {
                        printf("Connection closed.\n");
                        close(sockfd);
						return 0;
					} else if (strncmp(buffer, SUBSCRIBE, 10) == 0) {
                        strncpy(helper, buffer, strlen(buffer));
                        if (verify_subscribe(helper)) {
                            n = send(sockfd, buffer, strlen(buffer), 0);
                            DIE(n < 0, "send subscribe");
                        } else {
                            printf("Invalid subscribe format.\n");
                        }
                    } else if (strncmp(buffer, UNSUBSCRIBE, 12) == 0) {
                        strncpy(helper, buffer, strlen(buffer));
                        if (verify_unsubscribe(helper)) {
                            n = send(sockfd, buffer, strlen(buffer), 0);
                            DIE(n < 0, "send unsubscribe");
                        } else {
                            printf("Invalid unsubscribe format.\n");
                        }
                    } else {
                        printf("Invalid input.\n");
                    }
                } else if (i == sockfd) { // received message from server
                    memset(buffer, 0, BUFLEN);
					n = recv(sockfd, buffer, BUFLEN, 0);
					if (n == 0) {
                        printf("The server closed the connection.\n");
						close(sockfd);
						return 0;
					} else {
						printf("%s", buffer);
					}
                }
            }
        }
    }

    close(sockfd);
	return 0;
}

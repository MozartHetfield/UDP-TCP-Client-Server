#include "sun_lib.h"

using namespace std;

int main(int argc, char *argv[]) {

    int port_num;
    fd_set read_fds, tmp_fds;
    int tcpfd, udpfd, fdmax, connfd;
    socklen_t clilen;
    char buffer[BUFLEN], send_buffer[BUFLEN];
    struct sockaddr_in serv_addr_tcp, cli_addr, serv_addr_udp;
    int ret, n, flag; // flag is for Neagle

    map<int, int> sock_to_id;
    map<int, int> id_to_sock;
    map<int, tcp_client> id_to_client;
    map<string, topic> topic_name_to_content;

	DIE(argc != 2, "Usage: ./server <port>");
    port_num = atoi(argv[1]);
	DIE(port_num == 0, "atoi port");

    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcpfd < 0, "tcp socket");
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(tcpfd < 0, "udp socket");

    memset((char *) &serv_addr_tcp, 0, sizeof(serv_addr_tcp));
	serv_addr_tcp.sin_family = AF_INET;
	serv_addr_tcp.sin_port = htons(port_num);
	serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    memset((char *) &serv_addr_udp, 0, sizeof(serv_addr_udp));
	serv_addr_udp.sin_family = AF_INET;
	serv_addr_udp.sin_port = htons(port_num);
	serv_addr_udp.sin_addr.s_addr = INADDR_ANY;

    ret = bind(tcpfd, (struct sockaddr *) &serv_addr_tcp, sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");
    ret = bind(udpfd, (struct sockaddr *) &serv_addr_udp, sizeof(struct sockaddr));
	DIE(ret < 0, "bind udp");

    ret = listen(tcpfd, MAX_TCP_CLIENTS);
    DIE(ret < 0, "listen tcp");

    // Neagle for tcp
    flag = 1;
    ret = setsockopt(tcpfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    DIE(ret < 0, "Neagle");

    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(tcpfd, &read_fds);
    FD_SET(udpfd, &read_fds);
    fdmax = max(tcpfd, udpfd);

    while(1) {
        tmp_fds = read_fds;
        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == STDIN_FILENO) {
                    memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
                    if (strcmp(buffer, EXIT) == 0) { // close all connections and close the server
                        close(tcpfd);
                        close(udpfd);
                        FD_ZERO(&read_fds);
                        FD_ZERO(&tmp_fds);
                        printf("The server closed the connection.\n");
						return 0;
					}
                } else if (i == tcpfd) { // received tcp connection
                    clilen = sizeof(cli_addr);
                    connfd = accept(tcpfd, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(connfd < 0, "accept tcp connection");
                    FD_SET(connfd, &read_fds);
                    if (connfd > fdmax)
                        fdmax = connfd;
                    
                    // receive additional message with ID and port
                    memset(buffer, 0, BUFLEN);
					n = recv(connfd, buffer, sizeof(buffer), 0); // "<port_number> <ID>", always the same

                    int current_port = atoi(strtok(buffer, " ")); 
                    int current_id = atoi(strtok(NULL, " "));

                    // check if this ID si already logged in
                    if (id_to_sock[current_id] > 0) { // initially is 0 and it's set -1 in the disconnecting flow
                        memset(send_buffer, 0, BUFLEN);
                        strcpy(send_buffer, ALREADY_LOGGED);
                        n = send(connfd, send_buffer, BUFLEN, 0);
                        DIE(n < 0, "send already logged in");
                        close(connfd);
                        FD_CLR(connfd, &read_fds);
                        continue;
                    }

                    // Neagle
                    flag = 1;
                    ret = setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
                    DIE(ret < 0, "Neagle");

                    printf("New client %d connected from %s:%d.\n", current_id, inet_ntoa(cli_addr.sin_addr), current_port);

                    sock_to_id[connfd] = current_id;
                    id_to_sock[current_id] = connfd;
                    if (id_to_client.find(current_id) == id_to_client.end()) { // the first time this subscriber connects
                        id_to_client[current_id].id = current_id;
                    } else { // he is reconnecting

                        // flush queue
                        while (!id_to_client[current_id].sf_messages.empty()) {
                            string current_message = id_to_client[current_id].sf_messages.front();
                            id_to_client[current_id].sf_messages.pop();

                            n = send(connfd, current_message.c_str(), strlen(current_message.c_str()), 0);
                            DIE(n < 0, "flush queue");
                        }
                        
                        // set subscriber as active in his topics
                        for (unordered_set<string>::iterator it = id_to_client[current_id].topics.begin();
                                        it != id_to_client[current_id].topics.end(); ++it) {
                            
                            string current_topic = *it;
                            topic_name_to_content[current_topic].active_ids.insert(current_id);
                        }
                    }
                } else if (i == udpfd) { // received udp message TODO 
                    memset(buffer, 0, BUFLEN);
                    recvfrom(udpfd, buffer, BUFLEN, 0, (struct sockaddr*)&cli_addr, &clilen);

                    char* topic_name = (char*)malloc(TOPIC_SIZE);
                    strcpy(topic_name, buffer);

                    unsigned int type = buffer[TOPIC_SIZE];
                    char* content = (char*) malloc(BUFLEN);

                    // check message's type and store its final value in content
                    if (type == INT_ID) {
                        unsigned int sign = buffer[TOPIC_SIZE + 1];
                        char* char_value = (char*) malloc(4);
                        memcpy(char_value, buffer + TOPIC_SIZE + 2, 4);
                        uint32_t value = parse_int_udp_message(char_value, sign);
                        sprintf(content, "%d\0", value);
                        free(char_value);
                    } else if (type == SHORT_REAL_ID) {
                        char* char_value = (char*) malloc(2);
                        memcpy(char_value, buffer + TOPIC_SIZE + 1, 2);
                        char* value = parse_short_udp_message(char_value);
                        sprintf(content, "%s\0", value);
                        free(char_value);
                        free(value);
                    } else if (type == FLOAT_ID) {
                        unsigned int sign = buffer[TOPIC_SIZE + 1];
                        char* whole_number = (char*) malloc(4);
                        memcpy(whole_number, buffer + TOPIC_SIZE + 2, 4);
                        unsigned int floating_points = buffer[TOPIC_SIZE + 6];
                        char* value = parse_float_udp_message(whole_number, sign, floating_points);
                        sprintf(content, "%s\0", value);
                        free (whole_number);
                        free(value);
                    } else if (type == STRING_ID) {
                        strcpy(content, buffer + TOPIC_SIZE + 1);
                    }
                    strcat(content, "\n"); // for split messages on client

                    topic_name_to_content[topic_name].name = topic_name; //in case it doesn't exist

                    // send the message to active users
                    for (unordered_set<int>::iterator it = topic_name_to_content[topic_name].active_ids.begin();
                                        it != topic_name_to_content[topic_name].active_ids.end(); ++it) {
                        
                        int current_id = *it;
                        int current_socket = id_to_sock[current_id];

                        n = send(current_socket, content, strlen(content), 0);
                        DIE(n < 0, "topic sending");

                    }

                    // store the messages for inactive users that want to receive this on login
                    for (unordered_set<int>::iterator it = topic_name_to_content[topic_name].sf_ids.begin();
                                        it != topic_name_to_content[topic_name].sf_ids.end(); ++it) {
                        
                        int current_id = *it;

                        if (!topic_name_to_content[topic_name].active_ids.count(current_id)) {
                            int current_socket = id_to_sock[current_id];
                            id_to_client[current_id].sf_messages.push(content);
                        }
                    }
                    free(topic_name);
                    free(content);
                } else { // received a tcp message
                    memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
                    int current_id = sock_to_id[i];

                    if (n == 0) { // exit
                        printf("Client %d disconnected.\n", current_id);
                        id_to_sock[current_id] = -1;
                        close(i);
                        FD_CLR(i, &read_fds);

                        // remove him from active members in his topics
                        for (unordered_set<string>::iterator it = id_to_client[current_id].topics.begin();
                                        it != id_to_client[current_id].topics.end(); ++it) {

                            string current_topic = *it;
                            topic_name_to_content[current_topic].active_ids.erase(current_id);
                        }
                        continue;
                    }

                    char* command = buffer;
                    char* token = strchr(command, '\n');

                    while (token != NULL) { // split messages
                        *token++ ='\0';
                        if (strncmp(command, SUBSCRIBE, 10) == 0) {
                            strtok(command, " "); // subscribe
                            char* current_topic;
                            current_topic = strtok(NULL, " ");
                            int sf_value = atoi(strtok(NULL, "\n"));

                            // topic already subscribed by this user
                            if (id_to_client[current_id].topics.count(current_topic)) {
                                memset(send_buffer, 0, BUFLEN);
                                strcpy(send_buffer, ALREADY_SUB);
                                n = send(i, send_buffer, BUFLEN, 0);
                                DIE(n < 0, "send invalid subscribe");
                                continue;
                            }
                            topic_name_to_content[current_topic].name = current_topic;

                            topic_name_to_content[current_topic].active_ids.insert(current_id);
                            id_to_client[current_id].topics.insert(topic_name_to_content[current_topic].name);
                            
                            if (sf_value == 1) {
                                topic_name_to_content[current_topic].sf_ids.insert(current_id);
                            }

                        } else if (strncmp(command, UNSUBSCRIBE, 12) == 0) {
                            strtok(command, " "); // subscribe
                            char* current_topic;
                            current_topic = strtok(NULL, "\n");

                            if (id_to_client[current_id].topics.count(current_topic)) { // if I am already subscribed
                                
                                id_to_client[current_id].topics.erase(current_topic);
                                topic_name_to_content[current_topic].active_ids.erase(current_id);

                                if (topic_name_to_content[current_topic].sf_ids.count(current_id))
                                    topic_name_to_content[current_topic].sf_ids.erase(current_id);
                            } else {
                                memset(send_buffer, 0, BUFLEN);
                                strcpy(send_buffer, INVALID_UNSUB);
                                n = send(i, send_buffer, BUFLEN, 0);
                                DIE(n < 0, "send invalid unsubscribe");
                            }
                        }
                        command = token;
                        token = strchr(command, '\n');
                    }
                }
            }
        }
    }
	return 0;
}

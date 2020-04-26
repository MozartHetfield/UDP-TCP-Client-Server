#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <map>
#include <unordered_set>
#include <vector>
#include <string>
#include <queue>
#include <iostream>

#define ACTIVATE_DIE       1

#define MAX_TCP_CLIENTS    1000
#define BUFLEN             1500
#define TOPIC_SIZE         50
#define SAMPLE_SIZE        40

// udp messages helpers
#define INT_ID             0
#define SHORT_REAL_ID      1
#define FLOAT_ID           2
#define STRING_ID          3

// tcp client valid commands
#define EXIT               "exit\n"
#define SUBSCRIBE          "subscribe "
#define UNSUBSCRIBE        "unsubscribe "

// server input responses
#define ALREADY_SUB        "[SERVER] Topic already subscribed.\n"
#define INVALID_UNSUB      "[SERVER] You are not subscribed to this topic.\n"
#define ALREADY_LOGGED     "[SERVER] This ID is already logged in.\n"

using namespace std;

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion && ACTIVATE_DIE) {	\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

typedef struct tcp_client {
    int id;
    queue<string> sf_messages;
    unordered_set<string> topics;
} tcp_client;

typedef struct topic {
    string name;
    unordered_set<int> active_ids;
    unordered_set<int> sf_ids;
} topic;

int verify_subscribe(char* buffer);
int verify_unsubscribe(char* buffer);
char* parse_udp_message(char* buffer);
uint32_t parse_int_udp_message(char* char_value, unsigned int sign);
char* parse_short_udp_message(char* char_value);
char* parse_float_udp_message(char* char_value, unsigned int sign, unsigned int floating_points);
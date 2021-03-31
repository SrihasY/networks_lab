#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <stdint.h>
#include <inttypes.h>
#include <netinet/in.h>
#include<unistd.h>
#include <errno.h>

#define MAX_BUF 100
#define MAX_PEERS 5
#define MAX_NAME 256

struct user_entry {
    char username[MAX_NAME];
    char ip[16];
    uint16_t port;
    int cli_sockfd;
    struct sockaddr_in peer_addr;
};

struct user_info {
    struct user_entry table[MAX_PEERS];
    int number_peers;
};


void receive_messages(char* buf, fd_set* set);
void send_message(char* user, char* msg);
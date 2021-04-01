#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<inttypes.h>
#include<netinet/in.h>
#include<unistd.h>
#include<errno.h>
#include<sys/time.h>

#define MAX_BUF 512
#define MAX_PEERS 5
#define MAX_NAME 256
#define TIMEOUT 100

//entry of a peer user
struct user_entry {
    char username[MAX_NAME];	//username
    char ip[16];				//ip address of the peer's server
    uint16_t port;				//port on which the server runs
    int cli_sockfd;				//client socket corresponding to this server
    struct sockaddr_in peer_addr;//complete socket address info
    struct timeval timeout;		//timeout time for connection to this user
};

//user_info 
struct user_info {
    struct user_entry table[MAX_PEERS];//array to store info of all peers
    int number_peers;				   //no of peers
};


void receive_messages(char* buf, fd_set* set);
void send_message(char* user, char* msg);
void reset_timeout(struct timeval* timeout);
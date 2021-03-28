//Group 27
//18CS10057- Yarlagadda Srihas
//18CS30047- Somnath Jena
//Chat Application
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<limits.h>
#include<string.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<fcntl.h>

#include "chat.h"
#define SERV_PORT 1200

int serv_sockfd;
struct sockaddr_in serv_addr;

struct user_info user_info;

void print_user_info(void) {
    for(int i=0; i<user_info.number_peers; i++) {
        printf("Peer %d\n\tName: %s\n\tIP Address: %s\n\tPort: %u\n\n", i+1, user_info.table[i].username, user_info.table[i].ip, user_info.table[i].port);
    }
}

void start_server() {
    //set up the server address structure
	serv_addr.sin_family = AF_INET;                      //address family for IPv4
	serv_addr.sin_port = htons(SERV_PORT);                //Port number converted to network order
	inet_pton(AF_INET,"127.0.0.1",&(serv_addr.sin_addr));//convert string IP address to sin_addr
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));//to pad into struct sockaddr

    //retrieve a socket for the server
    if((serv_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		printf("The server socket descriptor could not be created. Error:%d. Exitting..\n",errno);
		exit(1);
	}

    //bind the socket to the port in SERVPORT
	if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
	{
		printf("Unable to bind socket to server port %d. Error:%d. Exiting..\n", SERV_PORT, errno);
        exit(1);
	}

	printf("Receiving at port %d.\n", SERV_PORT);
}

int main(void) {
    printf("**** Welcome to the P2P chat application ****\n");
    printf("Enter the number of peers: ");
    scanf("%d", &(user_info.number_peers));
    for(int i=0; i<user_info.number_peers; i++) {
        printf("Enter the username, IP address and port number of peer %d (space separated): ", i+1);
        struct user_entry* current = &(user_info.table[i]);
        scanf("%s %s %hu", current->username, current->ip, &current->port);
    }
    start_server();
    
    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    return 0;
}


//Group 27
//18CS10057- Yarlagadda Srihas
//18CS30047- Somnath Jena
//Chat Application
#include<limits.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<fcntl.h>
#include<sys/select.h>

#include "chat.h"
#define MAX_MSG 1000
#define MAX_IN 1200

uint16_t server_port;
int serv_sockfd;
struct sockaddr_in serv_addr;

struct user_info user_info;

int max(int a,int b)
{
	if(a>b)
		return a;
	return b;
}

void print_user_info(void) {
    for(int i=0; i<user_info.number_peers; i++) {
        printf("Peer %d\n\tName: %s\n\tIP Address: %s\n\tPort: %u\n\n", i+1, user_info.table[i].username, user_info.table[i].ip, user_info.table[i].port);
    }
}

void start_server(void) {
    //set up the server address structure
	serv_addr.sin_family = AF_INET;                      		
	serv_addr.sin_port = htons(server_port);                	
	inet_pton(AF_INET,"127.0.0.1",&(serv_addr.sin_addr));		
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));  

    //retrieve a socket for the server
    if((serv_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		printf("The server socket descriptor could not be created. Error:%d. Exiting..\n",errno);
		exit(1);
	}

    //bind the socket to the port in SERVPORT
	if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
	{
		printf("Unable to bind socket to server port %d. Error:%d. Exiting..\n", server_port, errno);
        exit(1);
	}

	//listen for incoming connections
	if(listen(serv_sockfd, MAX_PEERS)<0)
	{
		printf("Error in listening for connections. Error:%d. Exiting..\n",errno);
		exit(1);
	}

	printf("Receiving at port %d.\n", server_port);
}

int add_fds(fd_set* set) {
	FD_ZERO(set);
    FD_SET(serv_sockfd,set);
    FD_SET(STDIN_FILENO,set);
    int maxfd = serv_sockfd;
    for(int i=0;i<user_info.number_peers;i++)
    {
    	if(user_info.table[i].cli_sockfd>0)
    	{
    		FD_SET(user_info.table[i].cli_sockfd,set);
    		maxfd = max(maxfd,user_info.table[i].cli_sockfd);
    	}
    }
    return maxfd+1;
}

void accept_connection(void) {
	struct sockaddr_in cli_addr;
	int clisize = sizeof(cli_addr);
    int new_sockfd = accept(serv_sockfd, (struct sockaddr*)&cli_addr, &clisize);
    if(new_sockfd<0)
    {
    	printf("Error in accepting new connection. Error:%d. Exiting..\n",errno);
    	exit(1);
    }
    for(int i=0;i<user_info.number_peers;i++)
    {
    	struct user_entry* current = &(user_info.table[i]);
    	if(cli_addr.sin_addr.s_addr==current->peer_addr.sin_addr.s_addr)
    	{
    		current->cli_sockfd = new_sockfd;
			break;
    	} else {
			printf("Unknown client attempted to connect, rejected.\n");
		}
    }
}

void read_stdinput(char* std_input, char* buf, char* user, char* msg) {
	int read_size;
	int len=0;
    if((read_size=read(STDIN_FILENO,buf,MAX_BUF))>0)
    {
    	strcpy(std_input,buf);
    	len+=read_size;
    }
    int i;
    for(i=0;i<len;i++)
    {
    	if(std_input[i]=='/')
    		break;
    	user[i]=std_input[i];
    }
    user[i]='\0';
    int j=0;
    i++;
    for(;i<len;i++)
    {
    	msg[j]=std_input[i];
    	j++;
    }
    msg[j]='\0';
}
int main(void) {
    printf("**** Welcome to the P2P chat application ****\n");
	printf("Enter the server port: ");
	scanf("%hu", &server_port);
    printf("Enter the number of peers: ");
    scanf("%d", &(user_info.number_peers));
    for(int i=0; i<user_info.number_peers; i++) {
        printf("Enter the username, IP address and port number of peer %d (space separated):\n", i+1);
        struct user_entry* current = &(user_info.table[i]);
        scanf("%s %s %hu", current->username, current->ip, &current->port);
        current->cli_sockfd = -1;
		current->peer_addr.sin_family = AF_INET;                 
		current->peer_addr.sin_port = htons(current->port);
		if((current->peer_addr.sin_addr.s_addr = inet_addr(current->ip))==-1) {
			printf("Invalid IP address entered. Exiting...\n");
			exit(-1);
		}
		memset(current->peer_addr.sin_zero, '\0', sizeof(current->peer_addr.sin_zero));
    }
	print_user_info();
    start_server();
	
    fd_set readfds;

    int maxfd;
    int ready;
    char buf[MAX_BUF];
    char msg[MAX_MSG];
    char user[MAX_NAME];
    char std_input[MAX_IN];
    int read_size,sent_size;
    while(1){
		maxfd = add_fds(&readfds);
    	ready = select(maxfd,&readfds,NULL,NULL,NULL);
    	if(FD_ISSET(serv_sockfd,&readfds))
    	{
			accept_connection();
    	}
    	else if(FD_ISSET(STDIN_FILENO,&readfds))
    	{
    		read_stdinput(std_input, buf, user, msg);
			send_message(user,msg);
    	}
    	else
    	{
			receive_messages(buf, &readfds);
    	}
    }
    return 0;
}


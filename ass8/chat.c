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
#include<sys/select.h>

#include "chat.h"
#define SERV_PORT 1200
#define BACKLOG 5
#define MAX_BUF 100
#define MAX_USER 100
#define MAX_MSG 1000
#define MAX_IN 1200

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
        current->cli_sockfd = -1;
    }
    start_server();
    
    fd_set readfds;
    FD_ZERO(&readfds);
    //FD_ZERO(&writefds);

    int maxfd = serv_sockfd + 1;
    int ready, new_sockfd, cli_sockfd;
    struct sockaddr_in cli_addr;
    int clisize = sizeof(cli_addr);
    char buf[MAX_BUF];
    char msg[MAX_MSG];
    char user[MAX_USER];
    char input[MAX_IN];
    int read_size,sent_size;
    //listen for incoming connections
	if(listen(serv_sockfd, BACKLOG)<0)
	{
		printf("Error in listening for connections. Error:%d. Exitting..\n",errno);
		exit(1);
	}
    while(1){
    	FD_ZERO(&readfds);
    	//FD_ZERO(&writefds);
    	FD_SET(serv_sockfd,&readfds);
    	FD_SET(STDIN_FILENO,&readfds);
    	maxfd = serv_sockfd;
    	for(int i=0;i<user_info.number_peers;i++)
    	{
    		if(user_info.table[i].cli_sockfd>0)
    		{
    			//FD_SET(user_info.table[i].cli_sockfd,&writefds);
    			FD_SET(user_info.table[i].cli_sockfd,&readfds);
    			maxfd = max(maxfd,cli_sockfd);
    		}
    	}
    	maxfd++;
    	ready = select(maxfd,&readfds,NULL,NULL,NULL);

    	if(FD_ISSET(serv_sockfd,&readfds))
    	{
    		new_sockfd = accept(serv_sockfd, (struct sockaddr*)&cli_addr, &clisize);
    		if(new_sockfd<0)
    		{
    			printf("Error in accepting new connection. Error:%d. Exitting..\n",errno);
    			exit(1);
    		}
    		char ip[16];
    		uint16_t port=ntohs(cli_addr.sin_port);
    		inet_ntop(AF_INET,&(cli_addr.sin_addr),ip,16);
    		for(int i=0;i<user_info.number_peers;i++)
    		{
    			struct user_entry* current = &(user_info.table[i]);
    			if(port==current->port && !strcmp(ip,current->ip))
    			{
    				current->cli_sockfd = new_sockfd;
    			}
    		}
    	}
    	else if(FD_ISSET(STDIN_FILENO,&readfds))
    	{
    		char* std_input = input;
    		int len=0;
    		while((read_size=read(STDIN_FILENO,buf,MAX_BUF))>0)
    		{
    			strcpy(std_input,buf);
    			std_input+=read_size;
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
    		for(i=0;i<user_info.number_peers;i++)
    		{
    			struct user_entry* current = &(user_info.table[i]);
    			if(strcmp(current->username,user)==0)
    			{
    				if(current->cli_sockfd>0)
    				{
    					len = strlen(msg);
    					sent_size=0;
    					while(sent_size!=len)
    					{
    						sent_size+=send(current->cli_sockfd,msg+sent_size,len-sent_size,0);
    					}
    				}
    				else
    				{
    					if((current->cli_sockfd = socket(PF_INET, SOCK_STREAM,0))<0)
    					{
    						printf("The client socket descriptor could not be created. Error:%d. Exiting...\n", errno);
							exit(1);
    					}
    					struct sockaddr_in serv;
    					serv.sin_family = AF_INET;
    					serv.sin_addr.s_addr = inet_addr(current->ip);
    					memset(serv.sin_zero,'\0',sizeof(serv.sin_zero));
    					if(connect(current->cli_sockfd, (struct sockaddr *) &serv, sizeof(serv))==-1){
							printf("The connection could not be established. Error:%d. Exiting...\n", errno);
    					    close(current->cli_sockfd);
							exit(1);
						}
						FD_SET(current->cli_sockfd,&readfds);
						len = strlen(msg);
    					sent_size=0;
    					while(sent_size!=len)
    					{
    						sent_size+=send(current->cli_sockfd,msg+sent_size,len-sent_size,0);
    					}
    				}
    				break;
    			}
    		}
    	}
    	else
    	{
    		for(int i=0;i<user_info.number_peers;i++)
    		{
    			struct user_entry* current = &(user_info.table[i]);
    			if(current->cli_sockfd>0 && FD_ISSET(current->cli_sockfd,&readfds))
    			{
    				while((read_size=read(current->cli_sockfd,buf,MAX_BUF))>0)
    				{
    					printf("%s",buf);
    				}
    				printf("\n");
    			}
    		}
    	}
    }

    return 0;
}


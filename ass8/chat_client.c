#include "chat.h"
#include <arpa/inet.h>

extern struct user_info user_info;

void send_message(char* user, char* msg) {
    int sent_size, len;
    for(int i=0;i<user_info.number_peers;i++)
    {
    	struct user_entry* current = &(user_info.table[i]);
    	if(strcmp(current->username,user)==0)
    	{
    		if(current->cli_sockfd<0)
    		{
                // current->peer_addr.sin_family = AF_INET;                             
                // current->peer_addr.sin_port = htons(server_port);                    
                // inet_pton(AF_INET,"127.0.0.1",&(current->peer_addr.sin_addr));       
                // memset(current->peer_addr.sin_zero, '\0', sizeof(current->peer_addr.sin_zero));
    			if((current->cli_sockfd = socket(PF_INET, SOCK_STREAM,0))<0)
    			{
    				printf("The client socket descriptor could not be created. Error:%d. Exiting...\n", errno);
					exit(1);
    			}
                int yes=1;
                if(setsockopt(current->cli_sockfd,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT,&yes,sizeof(int))==-1){
                    printf("Error in port. Closing connection..\n");
                    exit(1);
                }
                if(bind(current->cli_sockfd, (struct sockaddr*)&(serv_addr), sizeof(serv_addr))<0)
                {
                    printf("Unable to bind socket to server port %d. Error:%d. Exiting..\n", server_port, errno);
                    exit(1);
                }
    			if(connect(current->cli_sockfd, (struct sockaddr *) &(current->peer_addr), sizeof(current->peer_addr))==-1){
					printf("The connection could not be established. Error:%d. Exiting...\n", errno);
    			    close(current->cli_sockfd);
					exit(1);
				}
    		}
            len = strlen(msg)+1;
    		sent_size=0;
    		while(sent_size!=len)
    		{
    			sent_size+=send(current->cli_sockfd,msg+sent_size,len-sent_size,0);
    		}
    		break;
    	}
    }
}

void receive_messages(char* buf, fd_set* set) {
    int read_size;
    for(int i=0;i<user_info.number_peers;i++)
    {
    	struct user_entry* current = &(user_info.table[i]);
    	if(current->cli_sockfd>0 && FD_ISSET(current->cli_sockfd,set))
    	{
			printf("%s: ", current->username);
    		while((read_size=recv(current->cli_sockfd,buf,MAX_BUF,0))>0)
    		{
				printf("%s",buf);
				if(buf[read_size-1]=='\0') {
					break;
				}
    		}
			if(read_size==0) {
				close(current->cli_sockfd);
				current->cli_sockfd=-1;
			}
    	}
    }
}
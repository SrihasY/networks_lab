//Group 27
//18CS10057- Yarlagadda Srihas
//18CS30047- Somnath Jena
//Client code
#include "chat.h"

extern int server_port;
extern struct sockaddr_in serv_addr;
extern struct user_info user_info;

//reset timeout time
void reset_timeout(struct timeval* timeout) {
	gettimeofday(timeout, NULL);
	timeout->tv_sec += TIMEOUT;
	timeout->tv_usec = 0;
}

//send a message read from stdin to corresponding user
void send_message(char* user, char* msg) {
    int sent_size, len;
    //loop through all user_info table entries
    for(int i=0;i<user_info.number_peers;i++)
    {
    	struct user_entry* current = &(user_info.table[i]);
    	//if username matched
    	if(strcmp(current->username,user)==0)
    	{
    		//if client socket does not exist
    		if(current->cli_sockfd<0)
    		{
    			//create client socket
    			if((current->cli_sockfd = socket(PF_INET, SOCK_STREAM,0))<0)
    			{

    				printf("The client socket descriptor could not be created. Error:%d. Exiting...\n", errno);
					exit(1);
    			}
				int yes=1;
				//to use the same port as this server's port
                if(setsockopt(current->cli_sockfd,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT,&yes,sizeof(int))==-1){
                    printf("Error in port. Closing connection..\n");
                    exit(1);
                }
                //bind the socket to the port in serv_port
                if(bind(current->cli_sockfd, (struct sockaddr*)&(serv_addr), sizeof(serv_addr))<0)
                {
                    printf("Unable to bind socket to server port %d. Error:%d. Exiting..\n", server_port, errno);
                    exit(1);
                }
                //try to connect to the corresponding server whose details are in the current entry
    			if(connect(current->cli_sockfd, (struct sockaddr *) &current->peer_addr, sizeof(current->peer_addr))==-1){
					if(errno==ECONNREFUSED) {
						//if connection is refused
						printf("The connection could not be made. The client may not be online.\n");
						//close the client socket fd
						close(current->cli_sockfd);
						//set client sockfd to -1
						current->cli_sockfd=-1;
						//break to prevent infinnite loop in sending loop
						break;
					} else {
						//if any other error exit the process
						printf("The connection could not be established. Error:%d. Exiting...\n", errno);
    			    	close(current->cli_sockfd);
						exit(1);
					}
				}
    		}
            len = strlen(msg);
    		sent_size=0;
    		//send length of msg bytes in the client socket
    		while(sent_size!=len)
    		{
    			sent_size+=send(current->cli_sockfd,msg+sent_size,len-sent_size,0);
    		}
    		//reset the timeout to current time of day + TIMEOUT
			reset_timeout(&current->timeout);
    		break;
    	}
    }
}

//receive message from users
void receive_messages(char* buf, fd_set* set) {
    int read_size;
    //loop through user_info table entries
    for(int i=0;i<user_info.number_peers;i++)
    {
    	struct user_entry* current = &(user_info.table[i]);
    	//if client socket exists and this socket is ready to be read from
    	if(current->cli_sockfd>0 && FD_ISSET(current->cli_sockfd,set))
    	{
			int firstbuf = 1;
			//receive the message
    		while((read_size=recv(current->cli_sockfd,buf,MAX_BUF,0))>0)
    		{
				if(firstbuf) {
					printf("%s: ", current->username);
				}
				buf[read_size]='\0';
				printf("%s",buf);
				if(buf[read_size-1]=='\n') {
					break;
				}
				firstbuf = 0;
    		}
    		//if no message received close the connection
			if(read_size==0) {
				close(current->cli_sockfd);
				current->cli_sockfd=-1;
			} else {
				//reset timeout to current timeout + TIMEOUT
				reset_timeout(&current->timeout);
			}
    	}
    }
}
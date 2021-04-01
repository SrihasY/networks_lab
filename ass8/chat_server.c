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
#include<signal.h>

#include "chat.h"

char server_ip[16];         //ip address of server on which the application runs
uint16_t server_port;       //poort on which server runs
int serv_sockfd;            //socket file descriptor
struct sockaddr_in serv_addr;//erver socket address data structure

struct user_info user_info; //information of peers connected to this server

//return maximum of 2 integers
int max(int a,int b)
{
	if(a>b)
		return a;
	return b;
}

//return minimum time of two times
time_t min_time(time_t a, time_t b)
{
	if(a<b)
		return a;
	return b;
}

//signal handler for CTRL+C(interrupt)
void sig_handler(int signo){
    if(signo==SIGINT)
    {
        printf("\nClosing connection & ending process..\n");
        close(serv_sockfd);
        exit(0);
    }
}

void print_user_info(void) {
    for(int i=0; i<user_info.number_peers; i++) {
        printf("Peer %d\n\tName: %s\n\tIP Address: %s\n\tPort: %u\n\n", i+1, user_info.table[i].username, user_info.table[i].ip, user_info.table[i].port);
    }
}

//function to start the application server running
void start_server(void) {
    //set up the server address structure
	serv_addr.sin_family = AF_INET;                              //address family for IPv4               		
	serv_addr.sin_port = htons(server_port);                     //Port number converted to network order    	
	inet_pton(AF_INET,server_ip,&(serv_addr.sin_addr));		     //convert string IP address to sin_addr
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));//to pad into struct sockaddr  

    //retrieve a socket for the server
    if((serv_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		printf("The server socket descriptor could not be created. Error:%d. Exiting..\n",errno);
		exit(1);
	}

	int yes=1;
    //to prevent hogging of port after closing and allow reusing of the server port for client to connect
    if(setsockopt(serv_sockfd,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT,&yes,sizeof(int))==-1){
        printf("Error in port. Closing connection..\n");
        exit(1);
    }

    //bind the socket to the port in server_port
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

//add all file descriptors to a read set
void add_fds(fd_set* set, int* maxfd, struct timeval* min_timeout) {
	FD_ZERO(set);                  //initialize file descriptor set
    FD_SET(serv_sockfd,set);       //add server socket file descriptor to the set
    FD_SET(STDIN_FILENO,set);      //add stdin file descriptor to the set
    *maxfd = serv_sockfd;          //initialize the maxfd to server socket fd
	min_timeout->tv_sec = LONG_MAX;//initialize min_timeout interval with LONG_MAX
	struct timeval current_time;   //to store current time 
	gettimeofday(&current_time, NULL);
    //loop through all user_info entries
    //and if corresponding client socket exists add it to the set
    for(int i=0;i<user_info.number_peers;i++)
    {
		struct user_entry* current_user = &(user_info.table[i]);
        //cli_sockfd<=0 indicates it has not been set up
    	if(current_user->cli_sockfd>0)
    	{
            //if corresponding client socket has timed out close the connection
			if(current_time.tv_sec >= current_user->timeout.tv_sec) {
				close(current_user->cli_sockfd);
				current_user->cli_sockfd = -1;
			} else {
                //add to fd set
    			FD_SET(user_info.table[i].cli_sockfd,set);
                //update maxfd
    			*maxfd = max(*maxfd,user_info.table[i].cli_sockfd);
                //update minimum timeout of all fds
				min_timeout->tv_sec = min_time(min_timeout->tv_sec, current_user->timeout.tv_sec - current_time.tv_sec);
			}
    	}
    }
	min_timeout->tv_usec=0;
	*maxfd = *maxfd+1;
    return;
}

//accept an incoming connection
void accept_connection(void) {
	struct sockaddr_in cli_addr;   //client address sturccture
	int clisize = sizeof(cli_addr);//sizeof sockaddr_in
    //accept an incoming connection
    int new_sockfd = accept(serv_sockfd, (struct sockaddr*)&cli_addr, &clisize);
    if(new_sockfd<0)
    {
    	printf("Error in accepting new connection. Error:%d. Exiting..\n",errno);
    	exit(1);
    }
	int valid=0;
    //loop to find out he user corresponding to cli_addr's ip and port
    for(int i=0;i<user_info.number_peers;i++)
    {
    	struct user_entry* current = &(user_info.table[i]);
    	if(cli_addr.sin_addr.s_addr==current->peer_addr.sin_addr.s_addr && cli_addr.sin_port==current->peer_addr.sin_port)
    	{
    		current->cli_sockfd = new_sockfd;
			reset_timeout(&current->timeout);
			valid=1;
			break;
    	}
    }
    //if any user other than the peers tries to connect, close the socket
	if(!valid) {
		close(new_sockfd);
		printf("Unknown client attempted to connect, rejected.\n");
	}		
}

//read from stdin
void read_stdinput(char* buf, char* user, char* msg) {
	int read_size;
	int len=0;
	char c;
    //read maximum of MAX_BUF no of characters and upto neewline only
    while((c=getchar())!='\n' && len<MAX_BUF) {
		buf[len++] = c;
	}
    //if message length larger than MAX_BUF discard characters after MAX_BUF
	if(len==MAX_BUF && c!='\n') {
		printf("Message input was longer than %d characters, it will be clipped.", MAX_BUF);
		buf[len-1]=='\n';
		while((c=getchar())!='\n') {
			;
		}
	} else {
		buf[len++]='\n';
	}
    int i;
    //find location of / to separate username and message
    for(i=0;i<len;i++)
    {
    	if(buf[i]=='/') {
			break;
		}	
    	user[i]=buf[i];
    }
    user[i]='\0';
    int j=0;
    i++;
    for(;i<len;i++)
    {
    	msg[j]=buf[i];
    	j++;
    }
    msg[j]='\0';
}

int main(void) {
	char filename[256];//filename of the file in which information of port and peers is stored
    printf("**** Welcome to the P2P chat application ****\n");
	printf("Enter the filename to retrieve user_info and server details: ");
	scanf("%s", filename);
	FILE* uinfofile = fopen(filename, "r");
	fscanf(uinfofile,"%s", server_ip);
	fscanf(uinfofile,"%hu", &server_port);
    fscanf(uinfofile,"%d", &(user_info.number_peers));
    //setup the user_info table
    for(int i=0; i<user_info.number_peers; i++) {
        struct user_entry* current = &(user_info.table[i]);
        fscanf(uinfofile,"%s %s %hu", current->username, current->ip, &current->port);
        current->cli_sockfd = -1;                           //initialize fd to -1
		current->peer_addr.sin_family = AF_INET;            //IPv4 address family     
		current->peer_addr.sin_port = htons(current->port); //port of the user applocation
		if((current->peer_addr.sin_addr.s_addr = inet_addr(current->ip))==-1) {
			printf("Invalid IP address in user_info. Exiting...\n");
			exit(-1);
		}
        //pad peer_addr with 0s
		memset(current->peer_addr.sin_zero, '\0', sizeof(current->peer_addr.sin_zero));
    }
    //setting up the CTRL+C signal handler
	signal(SIGINT, sig_handler);
	print_user_info();
    //start the server
    start_server();
	
    fd_set readfds;

    int maxfd;                  //max socket fd to pass in select()
	struct timeval min_timeout; //keep track of the minimum timeout of the fds in the readfds set
    int ready;                  //no of fds that are ready as returned from select
    char buf[MAX_BUF+1];        //buf to read from a fd
    char msg[MAX_BUF];          //message to send to an user
    char user[MAX_NAME];        //username
    int read_size,sent_size;    //no of bytes read in read() and sent in send()
    while(1){
        //add all necessary file descriptors to readfd set
		add_fds(&readfds, &maxfd, &min_timeout);
        //select the ready file descriptors
    	ready = select(maxfd,&readfds,NULL,NULL,&min_timeout);
		if(ready<=0)
			continue;
    	if(FD_ISSET(serv_sockfd,&readfds))
    	{
            //if server socket ready then accept connection
			accept_connection();
    	}
    	else if(FD_ISSET(STDIN_FILENO,&readfds))
    	{
            //if stdin is ready then read the input and send the message to corresponding user
    		read_stdinput(buf, user, msg);
			send_message(user,msg);
    	}
    	else
    	{
            //check which client sockets are ready
			receive_messages(buf, &readfds);
    	}
    }
    return 0;
}


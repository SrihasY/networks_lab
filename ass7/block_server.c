//Group 27
//18CS10057- Yarlagadda Srihas
//18CS30047- Somnath Jena
//Server File
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <limits.h>

#define SERVPORT 1200
#define MAXFILE 1000
#define MAXBUF 100
#define BACKLOG 1

//block size
int B=20;

//server socket file descriptor
int serv_sockfd;

//signal handler for CTRL+C(interrupt)
void sig_handler(int signo){
	if(signo==SIGINT)
	{
		printf("\nClosing connection & ending process..\n");
		close(serv_sockfd);
		exit(0);
	}
}

int main()
{
	//socket address data structures
	struct sockaddr_in serv_addr, cli_addr;
	int clisize = sizeof(cli_addr);
	int servsize= sizeof(serv_addr);
	//new socket file descriptor for a new connection
	int new_sockfd;
	//string to store filename sent by client
	char filename[MAXFILE];
	//buffer to store filename sent by client
	char filename_buf[MAXFILE];
	//buffer to send contents of file & other responses
	char buf[B];
	//file pointer to input file
	FILE* msgfile;
	//string to hold client's ip address
	char cli_ip[INET_ADDRSTRLEN];
	//file descriptor for filename sent
	int file_fd;
	int recv_size, sent_size, read_size;
	//struct to store file stats
	struct stat stbuf;
	//file size
	int FSIZE;
	//buffer to hold fsize
	char* fsize_buf;

	//setting up the CTRL+C signal handler
	signal(SIGINT,sig_handler);

	//open a socket on server side
	if((serv_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		printf("The server socket descriptor could not be created. Error:%d. Exitting..\n",errno);
		exit(1);
	}

	//set up the server address structure
	serv_addr.sin_family = AF_INET;                      //address family for IPv4
	serv_addr.sin_port = htons(SERVPORT);                //Port number converted to network order
	inet_pton(AF_INET,"127.0.0.1",&(serv_addr.sin_addr));//convert string IP address to sin_addr
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));//to pad into struct sockaddr

	//to prevent hogging of port after closing
	int yes=1;
	if(setsockopt(serv_sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1){
		printf("Error in port. Closing connection..\n");
		exit(1);
	}
	//bind the socket to the port in SERVPORT
	if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, servsize)<0)
	{
		printf("Unable to bind socket to server port %d. Error:%d. Exiting..\n", SERVPORT, errno);
        exit(1);
	}

	printf("Receiving at port %d.\n", SERVPORT);

	//listen for incoming connections
	if(listen(serv_sockfd, BACKLOG)<0)
	{
		printf("Error in listening for connections. Error:%d. Exitting..\n",errno);
		exit(1);
	}

	printf("Waiting for connections..\n");
	while(1){
		//accept a new connection
		new_sockfd = accept(serv_sockfd, (struct sockaddr*)&cli_addr, &clisize);
		if(new_sockfd<0)
		{
			printf("Error in accepting new connection. Error:%d. Exitting..\n",errno);
			exit(1);
		}

		filename[0]='\0';//initialize filename to null string
		//receive filename from client
		do{
			recv_size = recv(new_sockfd, filename_buf, MAXBUF, 0);
			if(recv_size==0)
				break;
			//concatenate current filename_buf content to filename
			strcat(filename,filename_buf);
			//if null reached then filename read completely
			if(filename_buf[recv_size-1]=='\0')
				break;
		}while(1);
		//if no filename received it means client has closed connection
		if(!recv_size)
		{
			//printf("Closing current connection..\n");
			close(new_sockfd);
			continue;
		}
		printf("Received filename. Opening file.. %s\n",filename);
		//opening file
		file_fd = open(filename,O_RDONLY,0666);
		//file not found
		if(file_fd<0)
		{
			//send error message
			sent_size=0;
			while((sent_size=send(new_sockfd,"E",1,0))<1)
			{
				if(sent_size==-1)
				{
					printf("Error in sending E. Error:%d. Exitting..\n",errno);
					exit(1);					
				}
			}
			//close current connection
			printf("File %s not found. Closing current connection..\n",filename);
			close(new_sockfd);
			continue;
		}
		else
		{
			stat(filename,&stbuf);
			//get size of the file
			FSIZE=stbuf.st_size;
			//send message L to client
			sent_size=0;
			while((sent_size=send(new_sockfd,"L",1,0))<1)
			{
				if(sent_size==-1)
				{
					printf("Error in sending L. Error:%d. Exitting..\n",errno);
					exit(1);					
				}
			}
			//store file size in a 4 byte variable FSIZE
			int32_t FSIZE_n=(int32_t)htonl(FSIZE);
			//cast to char* buffer
			fsize_buf=(char*)&FSIZE_n;
			sent_size=0;
			//send file size to client
			while(sent_size!=4)
			{
				sent_size+=send(new_sockfd,fsize_buf+sent_size,4-sent_size,0);
			}
		}

		//read from file in blocks of B size
		while((read_size=read(file_fd,buf,B))>0)
		{
			//send the current block
			sent_size=0;
			while(sent_size!=read_size)
			{
				sent_size += send(new_sockfd,buf+sent_size,read_size-sent_size,0);
			}			
		}
		close(file_fd);   //close file descriptor of the reading file
		close(new_sockfd);//close socket file descriptor
	}
	return 0;
}

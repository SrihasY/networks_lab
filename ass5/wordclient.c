//Group 27
//18CS10057- Yarlagadda Srihas
//18CS30047- Somnath Jena
//Client File
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<fcntl.h>

#define SERV_PORT 1200
#define MAXFILE 1000
#define MAXLINE 100

int main()
{
	int cli_sockfd;//client socket file descriptor
	int recvfd;    //file descriptor of the file to write to
	int numbytes;  //will be used to store no of bytes sent/received
	struct sockaddr_in serv_addr;//socket address information of server
	int serv_size=sizeof(serv_addr);
	char filename[MAXFILE];//string to hold filename to send
	char buf[MAXLINE];     //buffer to store received data / words
	char msg[MAXLINE];     //to store the messages that are to be sent to server

	//create a socket on the client side
	if((cli_sockfd = socket(PF_INET, SOCK_DGRAM, 0))<0){
		printf("The client socket descriptor could not be created. Error:%d. Exiting...\n", errno);
		exit(1);
	}

	//set up the socket address data of server
	serv_addr.sin_family = AF_INET;                              //address family for IPv4
	serv_addr.sin_port   = ntohs(SERV_PORT);                     //Port number converted to host order
	inet_pton(AF_INET, "127.0.0.1", &(serv_addr.sin_addr));      //convert
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));//to pad into struct sockaddr

	printf("Enter filename\n");
	scanf("%s",filename);
    
    //send filename to server
	if((numbytes=sendto(cli_sockfd,filename,strlen(filename)+1,0,
		(struct sockaddr*)&serv_addr,serv_size))==-1){
		printf("Error in sending filename. Error:%d. Exitting..\n",errno);
		exit(1);
	}
	else
		printf("File request sent to server\n");

    //receive response from server regarding the file
	if((numbytes=recvfrom(cli_sockfd,buf,MAXLINE,0,
		(struct sockaddr*)&serv_addr,&serv_size))==-1){
		printf("Error in receiving response from server. Error:%d. Exitting..\n",errno);
		exit(1);
	}
	else
	{
		printf("Response received\n");
		//check if response was FILE_NOT_FOUND from server
		if(strcmp(buf,"FILE_NOT_FOUND")==0)
		{
			printf("File Not Found\n");
			close(cli_sockfd);
			return 0;
		}// check if response was WRONG_FILE_FORMAT from server
		else if(strcmp(buf,"WRONG_FILE_FORMAT")==0)
		{
			printf("Wrong File Format\n");
			close(cli_sockfd);
			return 0;
		}// check if response was HELLO
		else if(strcmp(buf,"HELLO")==0)
		{
			//create a local file to write the words being received
			printf("Creating local file..\n");
			recvfd=open("received.txt",O_CREAT|O_WRONLY,0666);
			if(recvfd==-1){
				printf("Error in creating file. Exitting..\n");
				exit(1);
			}
			int wordcount=1;
			//write WORD_1 into msg to send to server
			sprintf(msg,"WORD_%d",wordcount);
			//send the current word to server
			numbytes=sendto(cli_sockfd,msg,strlen(msg)+1,0,
		    (struct sockaddr*)&serv_addr,serv_size);
		    //loop until END is received
		    do{
		    	//receive the next word from server in buf
		    	numbytes=recvfrom(cli_sockfd,buf,MAXLINE,0,
				(struct sockaddr*)&serv_addr,&serv_size);
				//if END is received
				if(strcmp(buf,"END")==0)
				{
					printf("Closing file..\n");
					close(recvfd);                   //close the local file
					printf("Closing connection..\n");
					close(cli_sockfd);               //close the client socket
					return 0;
				}
				strcat(buf,"\n");
				write(recvfd,buf,strlen(buf));       //write the received word into the local file
				wordcount++;                         //increase wordcount to send the next message
				sprintf(msg,"WORD_%d",wordcount);    //create the next message
				//send the next message
				numbytes=sendto(cli_sockfd,msg,strlen(msg)+1,0,
		    	(struct sockaddr*)&serv_addr,serv_size);
		    }while(1);
		}		
		else
		{
			printf("No data received. Closing connection..\n");
			close(cli_sockfd);
		}
	}
	return 0;
}
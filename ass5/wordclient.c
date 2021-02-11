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
	int cli_sockfd;
	int recvfd;
	int status;
	int numbytes;
	struct sockaddr_in serv_addr,cli_addr;
	int serv_size=sizeof(serv_addr);
	char filename[MAXFILE];
	char buf[MAXLINE];
	char word[MAXLINE];
	//char recv_word[MAXLINE];

	if((cli_sockfd = socket(PF_INET, SOCK_DGRAM, 0))<0){
		printf("The client socket descriptor could not be created. Error:%d. Exiting...\n", errno);
		exit(1);
	}
	else{
		printf("The client socket descriptor was successfully created\n");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port   = ntohs(SERV_PORT);
	inet_pton(AF_INET, "127.0.0.1", &(serv_addr.sin_addr));

	printf("Enter filename\n");
	scanf("%s",filename);

	if((numbytes=sendto(cli_sockfd,filename,strlen(filename),0,
		(struct sockaddr*)&serv_addr,serv_size))==-1){
		printf("Error in sending filename. Error:%d. Exitting..\n",errno);
		exit(1);
	}
	else
		printf("File request sent to server\n");

	if((numbytes=recvfrom(cli_sockfd,buf,MAXLINE,0,
		(struct sockaddr*)&serv_addr,&serv_size))==-1){
		printf("Error in receiving response from server. Error:%d. Exitting..\n",errno);
		exit(1);
	}
	else
	{
		if(strcmp(buf,"FILE_NOT_FOUND")==0)
		{
			printf("File Not Found\n");
			close(cli_sockfd);
			return 0;
		}
		else if(strcmp(buf,"WRONG_FILE_FORMAT")==0)
		{
			printf("Wrong File Format\n");
			close(cli_sockfd);
			return 0;
		}
		else if(strcmp(buf,"HELLO")==0)
		{
			recvfd=open("received.txt",O_APPEND|O_CREAT|O_WRONLY,0666);
			if(recvfd==-1){
				printf("Error in creating file. Exitting..\n");
				exit(1);
			}
			int i=1;
			sprintf(word,"WORD_%d",i);
			numbytes=sendto(cli_sockfd,word,strlen(word),0,
		    (struct sockaddr*)&serv_addr,serv_size);
		    do{
		    	numbytes=recvfrom(cli_sockfd,buf,MAXLINE,0,
				(struct sockaddr*)&serv_addr,&serv_size);
				if(strcmp(buf,"END")==0)
				{
					printf("Closing file..\n");
					close(recvfd);
					printf("Closing connection..\n");
					close(cli_sockfd);
					return 0;
				}
				i++;
				strcat(buf,"\n");
				write(recvfd,buf,strlen(buf));
				sprintf(word,"WORD_%d",i);
				numbytes=sendto(cli_sockfd,word,strlen(word),0,
		    	(struct sockaddr*)&serv_addr,serv_size);
		    }while(1);
		}		
		else
		{
			printf("No data received. Closing connection..\n");
			close(cli_sockfd);
		}
	}
	//close(cli_sockfd);
	return 0;
}
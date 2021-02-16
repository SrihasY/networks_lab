//Group 27
//18CS10057- Yarlagadda Srihas
//18CS30047- Somnath Jena
//Server File
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<fcntl.h>

#define BUFSIZE 60
#define MAXFILE 256
#define SERV_PORT 1200

int main(void) {
    //client socket file descriptor
    int cli_sockfd;
    //file descriptor of the file to write to
	int recvfd;
    //will be used to store count of bytes received
	int byte_count=0;
    //number of words received
    int word_count=0;
    //state bit to check if we are parsing a word
    bool in_word;
    //server socket address struct
	struct sockaddr_in serv_addr;
    //string to hold filename to send
	char filename[MAXFILE];
    //buffer to store received data
	char buf[60];

	//create a socket on the client side
	if((cli_sockfd = socket(PF_INET, SOCK_STREAM, 0))<0){
		printf("The client socket descriptor could not be created. Error:%d. Exiting...\n", errno);
		exit(1);
	}

    //set up the socket address data of server
	serv_addr.sin_family = AF_INET;                 
	serv_addr.sin_port   = ntohs(SERV_PORT);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));

    printf("Enter filename:\n");
	scanf("%s",filename);

    //attempt to connect to the server
    if(connect(cli_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))==-1){
		printf("The connection could not be established. Error:%d. Exiting...\n", errno);
        close(cli_sockfd);
		exit(1);
	}

    printf("Connection established at port 1200 of 127.0.0.1.\n");
    printf("Sending file request...");

    //send the filename to the server
    char* file_sent = filename; 
    char* end = filename+strlen(filename)+1;
    int cur_sent;
    while(file_sent!=end) {
        if((cur_sent=send(cli_sockfd, file_sent, end-file_sent, 0))==-1) {
            printf("\nThe file request could not be sent. Error:%d. Exiting...\n", errno);
            close(cli_sockfd);
		    exit(1);
        }
        file_sent+=cur_sent;
    }

    printf("file request sent.\n");

    //receive data from the server
    int recv_bytes;
    if(((recv_bytes=recv(cli_sockfd, buf, BUFSIZE, 0))>0)) {
        //create the output file
        recvfd = open("received.txt", O_CREAT | O_WRONLY, 0666);
        if(recvfd==-1) {
            printf("The output file could not be created. Error:%d. Exiting...\n", errno);
            close(cli_sockfd);
		    exit(1);
        }
        //initialize the word flag
        in_word = false;
        //read the data in chunks
        do {
            //write the chunk to the output file
            if(write(recvfd, buf, recv_bytes)==-1) {
                printf("Data could not be written to the output file. Error:%d. Exiting...\n", errno);
                close(cli_sockfd);
                close(recvfd);
		        exit(1);
            }
            //parse the chunk to count words
            for(int i=0; i<recv_bytes; i++) {
                char c = buf[i];
                if(c==','||c==';'||c==':'||c=='.'||c=='\t'||c=='\n'||c==' ') {
                    //unset word flag
                    in_word=false;
                } else if(!in_word){
                    //increment word count
                    word_count++;
                    //set word flag
                    in_word=true;
                }
            }
            //update the total byte count
            byte_count+=recv_bytes;
        } while((recv_bytes=recv(cli_sockfd, buf, BUFSIZE, 0))>0);
    }

    if(recv_bytes==-1) {
        //error while receiving data
        printf("There was an error receiving data from the server. Error:%d. Exiting...\n", errno);
        close(cli_sockfd);
        if(byte_count>0)
            close(recvfd);
        exit(1);
    } else if(recv_bytes==0 && byte_count==0) {
        //server closed connection without sending any data
        printf("ERR 01: File Not Found\n");
        close(cli_sockfd);
        exit(1);
    } else {
        //file received successfully
        printf("The file transfer is successful. Size of the file = %d bytes, no. of words = %d\n", 
        byte_count, word_count);
    }
    close(cli_sockfd);
    close(recvfd);
    return 0;
}
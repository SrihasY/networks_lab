//Group 27
//18CS10057- Yarlagadda Srihas
//18CS30047- Somnath Jena
//Client File
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

#define BLOCK_SIZE 20
#define MAX_MESSAGE 16
#define MAXFILE 256
#define SERV_PORT 1200

int main(void) {
    //client socket file descriptor
    int cli_sockfd;
    //file descriptor of the file to write to
	int recvfd;
    //integer to store the number of blocks received
    int block_count=0;
    //integer to store the size of the last block
    int lblock_size;
    //integer to store the size of the file to be received
    int fsize;
    //server socket address struct
	struct sockaddr_in serv_addr;
    //string to hold filename to send
	char filename[MAXFILE];
    //string to store the first message sent by the server
    char message[MAX_MESSAGE];
    //buffer to store a block of received data
	char buf[BLOCK_SIZE];

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
    //send repeatedly until the full filename is transferred
    while(file_sent!=end) {
        if((cur_sent=send(cli_sockfd, file_sent, end-file_sent, 0))==-1) {
            printf("\nThe file request could not be sent. Error:%d. Exiting...\n", errno);
            close(cli_sockfd);
		    exit(1);
        }
        file_sent+=cur_sent;
    }

    printf("file request sent.\nWaiting for server message...");

    //receive the message from the server
    int recv_bytes;
    //THIS RECV MAY RECEIVE BITS FROM THE FIRST BLOCK, SINCE THERE IS NO ACK/MSG_WAITALL
    //TBD
    while(((recv_bytes=recv(cli_sockfd, buf, MAX_MESSAGE, 0))>0)) {
        strcat(message, buf);
        if(buf[recv_bytes-1]=='\0') {
            break;
        }
    }

    if(recv_bytes<0) {
        printf("\nThe server message could not be received. Error:%d. Exiting...\n", errno);
        close(cli_sockfd);
		exit(1);
    } else {
        printf("server message received.\n");
        if(message[0]=='L') {
            //trim the message to retrieve only file size
            memmove(message, message+1, strlen(message));
            //convert the file size into integer
            char* endptr;
            long flong = strtol(message, endptr, 10);
            if(endptr == message || *endptr!='\0' || errno == ERANGE || flong > INT_MAX) {
                printf("The file size received is invalid. Exiting...\n");
                close(cli_sockfd);
		        exit(1);
            }
            //update the file size variable
            fsize = (int) flong;
        } else if(message[0]=='E' && strlen(message)==1) {
            //if 'E' message is received, the file was not found
            printf("The server returned an error message - File Not Found. Exiting...\n");
            close(cli_sockfd);
		    exit(1);
        } else {
            printf("The server message was malformed. Exiting...\n");
            close(cli_sockfd);
		    exit(1);
        }
    }

    //create the output file since the server message was received successfully
    recvfd = open("received.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if(recvfd==-1) {
        printf("The output file could not be created. Error:%d. Exiting...\n", errno);
        close(cli_sockfd);
	    exit(1);
    }

    //receive the data in blocks
    while(block_count <= fsize/BLOCK_SIZE) {
        //number of bytes to wait for
        int expected;
        if(block_count == fsize/BLOCK_SIZE) {
            //check to see if the last block has been received
            if(fsize%BLOCK_SIZE == 0) {
                if(fsize == 0) {
                    //empty file
                    lblock_size = 0;
                } else {
                    //last block already received
                    lblock_size = BLOCK_SIZE;
                }
                break;
            } else {
                //set recv bytes to last block expected size
                expected = fsize%BLOCK_SIZE;
            }
        } else {
            //set recv bytes to block size
            expected = BLOCK_SIZE;
        }
        recv_bytes = recv(cli_sockfd, buf, expected, MSG_WAITALL);
        if(recv_bytes < 0) {
            printf("Error while receiving data. Exiting...\n");
            close(cli_sockfd);
            close(recvfd);
            exit(1);
        } else if(recv_bytes < expected) {
            printf("Received an incorrectly sized block. Exiting...\n");
            close(cli_sockfd);
            close(recvfd);
            exit(1);
        }
        //if the block was received correctly, write to file
        if(write(recvfd, buf, recv_bytes)==-1) {
            printf("Data could not be written to the output file. Error:%d. Exiting...\n", errno);
            close(cli_sockfd);
            close(recvfd);
            exit(1);
        }
        //update the received block count
        block_count++;
        //update the last received block size
        lblock_size = recv_bytes;
    }
    //close socket and output file descriptors
    close(cli_sockfd);
    close(recvfd);

    //file received successfully
    printf("The file transfer is successful. Total number of blocks received = %d bytes, Last block size = %d.\n",
          block_count, lblock_size);
    return 0;
}
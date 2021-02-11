#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERV_PORT 1200
#define MAX_CON 10
#define MAXFILE 1000

int main(void) {
    //socket address structures
    struct sockaddr_in serv_addr, cli_addr;
    int clisize = sizeof(cli_addr);
    //server socket file descriptor
    int serv_sockfd;
    //string to store the filename of the input file
    char filename[MAXFILE];
    //buffer for incoming commands and outgoing messages
    char buf[MAXFILE];
    //file pointer to the input file
    FILE* msgfile;

    //open a socket on the server
    if((serv_sockfd=socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("The server socket descriptor could not be created. Error:%d. Exiting...\n", errno);
        exit(1);
    }
    
    //set up the server socket address struct
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));

    //bind the socket to the port specified in SERV_PORT
    if(bind(serv_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Unable to bind socket to server port %d. Error:%d. Exiting...\n", SERV_PORT, errno);
        exit(1);
    }

    printf("Receiving at port %d.\n", SERV_PORT);

    //read the filename sent by the client
    recvfrom(serv_sockfd, filename, MAXFILE, 0, (struct sockaddr *) &cli_addr, &clisize);
    printf("Request received. Opening file \"%s\"...\n", filename);

    //attempt to open the file
    msgfile = fopen(filename, "r");
    
    //send error message if the file is not found
    if(msgfile==NULL && errno==ENOENT) {
        char fnotfound[] = "FILE_NOT_FOUND";
        sendto(serv_sockfd, fnotfound, strlen(fnotfound)+1, 0, (struct sockaddr *) &cli_addr, clisize);
        printf("The file requested was not found. Closing connection...\n");
        close(serv_sockfd);
        return 0;
    }

    //scanf the first word from the file
    fscanf(msgfile, "%s", buf);
    //send error message if the first word is not HELLO
    if(strcmp(buf, "HELLO")!=0) {
        char wformat[] = "WRONG_FILE_FORMAT";
        sendto(serv_sockfd, wformat, strlen(wformat)+1, 0, (struct sockaddr *) &cli_addr, clisize);
        printf("Wrong file format. Closing connection...\n");
        fclose(msgfile);
        close(serv_sockfd);
        return 0;
    }
    
    //send HELLO to the client
    sendto(serv_sockfd, buf, strlen(buf)+1, 0, (struct sockaddr *) &cli_addr, clisize);

    int wordcount=1;
    while(1) {
        //receive the next commmand from the client
        if(recvfrom(serv_sockfd, buf, MAXFILE, 0, (struct sockaddr *) &cli_addr, &clisize) < 0) {
            printf("Read from client failed. Error:%d. Exiting...\n", errno);
            fclose(msgfile);
            close(serv_sockfd);
            exit(1);
        }

        //generate the next expected command
        char word[MAXFILE] = "WORD_";
        int length = snprintf(NULL, 0, "%d", wordcount);
        char* wnum = malloc(sizeof(char)*(length+1));
        snprintf(wnum, length+1, "%d", wordcount);
        strcat(word, wnum);
        
        //send an error message if the word command is incorrect
        if(strcmp(word, buf)!=0) {
            char wmismatch[] = "INCORRECT_COMMAND";
            sendto(serv_sockfd, wmismatch, strlen(wmismatch)+1, 0, (struct sockaddr *) &cli_addr, clisize);
            printf("Incorrect command. Closing connection...\n");
            fclose(msgfile);
            close(serv_sockfd);
            return 0;
        }

        //read the next word from the file
        if(fscanf(msgfile, "%s", buf) < 1) {
            //close the connection if file ends before keyword END is found
            char ueof[] = "UNEXPECTED_EOF";
            sendto(serv_sockfd, ueof, strlen(ueof)+1, 0, (struct sockaddr *) &cli_addr, clisize);
            printf("Unexpected end of file. Closing connection...\n");
            fclose(msgfile);
            close(serv_sockfd);
            exit(1);
        } else {
            //send the next word to the client
            sendto(serv_sockfd, buf, strlen(buf)+1, 0, (struct sockaddr *) &cli_addr, clisize);
            //finish if END is read
            if(strcmp("END", buf)==0) {
                break;
            }
        }
        wordcount++;
    }
    printf("Process completed. Closing connection and exiting...\n");
    fclose(msgfile);
    close(serv_sockfd);
    return 0;
}
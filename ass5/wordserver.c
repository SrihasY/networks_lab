#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERV_PORT 1200
#define MAX_CON 10
#define MAXFILE 1000

int main(void) {
    struct sockaddr_in serv_addr;
    int serv_sockfd;
    if((serv_sockfd=socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("The server socket descriptor could not be created. Error:%d. Exiting...\n", errno);
        exit(1);
    }
    
    //set up the server socket address struct
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));

    //bind the socket to the port specified in SERV_PORT
    if(bind(serv_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Unable to bind socket to server port %d. Error:%d. Exiting...\n", SERV_PORT, errno);
        exit(1);
    }

    printf("Receiving at port %d.\n", SERV_PORT);

    struct sockaddr_in cli_addr;
    int clisize = sizeof(cli_addr);

    clisize = sizeof(cli_addr);
    char filename[MAXFILE];
    recvfrom(serv_sockfd, filename, MAXFILE, 0, (struct sockaddr *) &cli_addr, &clisize);
    filename[strlen(filename)-1]='\0';
    FILE* msgfile = fopen(filename, "r");
    if(msgfile==NULL && errno==ENOENT) {
        char fnotfound[] = "FILE_NOT_FOUND";
        sendto(serv_sockfd, fnotfound, strlen(fnotfound), 0, (struct sockaddr *) &cli_addr, clisize);
        printf("The file requested was not found. Closing connection...\n");
        close(serv_sockfd);
        return 0;
    }

    char buf[MAXFILE];
    fscanf(msgfile, "%s", buf);
    if(strcmp(buf, "HELLO")!=0) {
        char wformat[] = "WRONG_FILE_FORMAT";
        printf("Wrong file format. Closing connection...\n");
        close(serv_sockfd);
        return 0;
    }
    
    //send HELLO to the client
    sendto(serv_sockfd, buf, strlen(buf), 0, (struct sockaddr *) &cli_addr, clisize);

    clisize = sizeof(cli_addr);
    int wordcount=1;
    while(1) {
        if(recvfrom(serv_sockfd, buf, MAXFILE, 0, (struct sockaddr *) &cli_addr, &clisize) < 0) {
            printf("Read from client failed. Error:%d. Exiting...\n", errno);
            exit(1);
        }

        char word[MAXFILE] = "WORD";
        int length = snprintf( NULL, 0, "%d", wordcount );
        char* wnum = malloc(sizeof(char)*(length+1));
        snprintf( wnum, length+1, "%d", wordcount);
        strcat(word, wnum);
        
        buf[strlen(buf)-1]='\0';

        if(strcmp(word, buf)!=0) {
            char wmismatch[] = "Incorrect WORD command.";
            sendto(serv_sockfd, wmismatch, strlen(wmismatch), 0, (struct sockaddr *) &cli_addr, clisize);
            continue;
        }

        if(fscanf(msgfile, "%s", buf) < 1) {
            char wformat[] = "UNEXPECTED_EOF";
            printf("Unexpected end of file. Closing connection...\n");
            close(serv_sockfd);
            return 0;
        } else {
            sendto(serv_sockfd, buf, strlen(buf), 0, (struct sockaddr *) &cli_addr, clisize);
            if(strcmp("END", buf)==0) {
                break;
            }
        }
        wordcount++;
    }
    
    printf("Process completed. Closing connection and exiting...\n");
    close(serv_sockfd);
    return 0;
}

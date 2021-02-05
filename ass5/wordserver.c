#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define SERV_PORT 3500
#define MAX_CON 10
#define MAXFILE 1000

int main(void) {
    struct sockaddr_in serv_addr;
    int serv_sockfd;
    if(serv_sockfd=socket(PF_INET, SOCK_DGRAM, 0) < 0) {
        printf("The server socket descriptor could not be created. Exiting...\n");
        exit(1);
    }
    
    //set up the server socket address struct
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));

    //bind the socket to the port specified in SERV_PORT
    if(bind(serv_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Unable to bind socket to server port %d. Exiting...\n", SERV_PORT);
        exit(1);
    }

    //listen for incoming connections
    if(listen(serv_sockfd, MAX_CON) < 0) {
        printf("Error while listening for incoming connections at port %d. Exiting...\n", SERV_PORT);
        exit(1);
    }

    printf("Listening for connections at port %d...\n", SERV_PORT);

    struct sockaddr_in cli_addr;
    int clisize = sizeof(cli_addr);
    int conn_sockfd;
    if((conn_sockfd = accept(serv_sockfd, (struct sockaddr *) &cli_addr, &clisize)) < 0) {
        printf("Unable to accept connection at port %d. Exiting...\n", SERV_PORT);
        exit(1);
    }
    
    printf("Connection established.\n");

    clisize = sizeof(cli_addr);
    char filename[MAXFILE];
    recvfrom(conn_sockfd, filename, MAXFILE, 0, (struct sockaddr *) &cli_addr, &clisize);
    FILE* msgfile = fopen(filename, "r");
    if(msgfile==NULL) {
        
    }

    return 0;
}

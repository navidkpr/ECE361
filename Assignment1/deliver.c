#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MYPORT "3469"

int main( int argc, char *argv[] )
{
    char * serverAddress = argv[1];
    char * serverPortNum= argv[2];
    char * proto[4] = "ftp";
    char * fileName;

    // if (argc != 3) {
    //     fprintf(stderr,"usage: deliver ServerAddress ServerPortNumber\n");
    //     exit(1);
    // }
    
    // scanf("%s %s", proto, fileName);
    //TODO: Check existence of fileName

    int sockfd;
    struct addrinfo hints, *servinfo;
    int rv;
    int numBytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    //-----------------------------------------------------------------------------------------------------------

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sockfd < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    if ((numBytes = sendto(sockfd, proto, strlen(proto), 0,
        servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
        perror("deliver: sendto");
        exit(1);
    }


    freeaddrinfo(servinfo);
    printf("deliver: sent %d bytes to server\n", numBytes);
    close(sockfd);

    return 0;
}
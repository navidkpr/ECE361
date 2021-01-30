#include <stdio.h>
#include <string.h>

#define MYPORT "3490"

int main( int argc, char *argv[] )
{
    char * serverAddress = argv[1];
    char * serverPortNum= argv[2];
    //-----------------------------------------------------------------------------------------------------------

    getaddrinfo(NULL, MYPORT, &hints, &res);

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        printf("Couldn't bind to the port\n");
        return -1;
    }


    return 0;
}
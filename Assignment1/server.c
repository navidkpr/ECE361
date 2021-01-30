#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MYPORT "3470"  // the port users will be connecting to
#define BACKLOG 10 

int main() {
    struct sockaddr_storage client_addr;
    
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

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

    printf("Done with binding\n");

    while (1) {
        printf("Listening for incoming messages...\n\n");


        addr_size = sizeof client_addr;
        char client_message[3000];
        if (recvfrom(sockfd, client_message, sizeof(client_message), 0, (struct sockaddr*)&client_addr, &addr_size) < 0) {
            printf("Recieve Error\n");
            return -1;
        }

        char *response;
        if (client_message == "ftp")
            response = "yes";
        else
            response = "no";

        printf("Msg from client: %s\n", client_message);
        
        if (sendto(sockfd, response, strlen(response), 0,
            (struct sockaddr*)&client_addr, &addr_size) < 0){
            printf("Reesponse Error\n");
            return -1;
        }
    }

    close(sockfd);
}
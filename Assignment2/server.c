#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BACKLOG 10 
#define MYHOST "ug136"

FILE * fPtr;

int create_file_from_packet(char *str) {
    // char *str = "32:2:10:foobar.txt:lo World!\n";
    
    int total_frag = 0;
    int frag_no = 0;
    int packet_size = 0;

    int pt = 0;
    while (str[pt] != ':') {
        total_frag = total_frag * 10 + str[pt] - '0';
        pt++;
    }

    pt++;
    while (str[pt] != ':') {
        frag_no = frag_no * 10 + str[pt] - '0';
        pt++;
    }

    pt++;
    while (str[pt] != ':') {
        packet_size = packet_size * 10 + str[pt] - '0';
        pt++;
    }

    pt++;
    int st_pt = pt;
    while (str[pt] != ':')
        pt++;


    char *file_name = malloc( sizeof(char) * ( pt - st_pt + 1 ) );
    memcpy (file_name, &str[st_pt], pt - st_pt);
    file_name[pt - st_pt] = '\0';
    




    pt++;
    st_pt = pt;
    
    char *content = malloc( sizeof(char) * ( packet_size + 1 ) );
    memcpy (content, &str[st_pt], packet_size);
    content[packet_size] = '\0';

    printf("%d %d %d\n", total_frag, frag_no, packet_size);
    puts(file_name);
    puts(content);


    
    if (frag_no == 1)
        fPtr = fopen(file_name, "w");
    // else
    //     fPtr = fopen(file_name, "a");

    fputs(content, fPtr);

    
    if (frag_no == total_frag){
        fclose(fPtr);
        return 1;
    }
        
    
    return 0;
}

int main( int argc, char *argv[] ) {
    if (argc != 2){
        fprintf(stderr,"usage: server ServerPortNumber\n");
        exit(1);
    }
    char * serverPortNum = argv[1];
    struct sockaddr_storage client_addr;
    
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;
    int recieveNumBytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(MYHOST, serverPortNum, &hints, &res);

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

    printf("Listening for incoming messages...\n\n");

    int isDone = 0;

    while (!isDone) {
        addr_size = sizeof client_addr;
        char packet[1000];
        if ((recieveNumBytes = recvfrom(sockfd, packet, sizeof(packet), 0, (struct sockaddr*)&client_addr, &addr_size)) < 0) {
            printf("Recieve Error\n");
            return -1;
        }


        printf("Packet is: %s\n", packet);
        isDone = create_file_from_packet(packet);

        char *response;

        //if packet is an actual segment of the file:
        response = "ACK"; 

        
        
        if (sendto(sockfd, response, strlen(response), 0,
            (struct sockaddr*)&client_addr, addr_size) < 0){
            printf("Reesponse Error\n");
            return -1;
        }
    }

    close(sockfd);
}
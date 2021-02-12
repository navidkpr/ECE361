#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <math.h>


struct Packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

int main( int argc, char *argv[] ) //Run program with deliver.o LocalHost 3470
{
    clock_t start, end;
    double cpu_time_used;
    char * serverAddress = argv[1];
    char * serverPortNum= argv[2];
    //char * proto = "ftp";
    char inputPre[100];

    if (argc != 3) {
        fprintf(stderr,"usage: deliver ServerAddress ServerPortNumber\n");
        exit(1);
    }
    
    
    fgets(inputPre, sizeof(inputPre), stdin);
    char * inputPost = strtok(inputPre, "\n");
    char * proto = strtok(inputPost, " ");
    if (proto == NULL){
        exit(1);
    }
    char * fileName = strtok(NULL, " ");
    // printf("The proto length is %d\n", strlen(proto));
    // printf("The fileName length is %d\n", strlen(fileName));
    
    // puts (proto);
    // puts(fileName);

    if (access(fileName, F_OK) != 0){
        exit(1);
    }


    struct Packet packet;
    packet.frag_no = 1;
    packet.filename = fileName;

    FILE *fileptr;
    long filelen;
    fileptr = fopen(packet.filename,"rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);
    packet.total_frag = (long)ceil((double) filelen/1000);
    


    int sockfd;
    struct addrinfo hints, *servinfo;
    int rv;
    int sendNumBytes;
    int recieveNumBytes;
    char buffer[1000];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    

    //-----------------------------------------------------------------------------------------------------------


    if ((rv = getaddrinfo(serverAddress, serverPortNum, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sockfd < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    int x;
    for (x = 1; x <= packet.total_frag; x++){
        packet.frag_no = (unsigned int)x;
        if (filelen > 1000){
            packet.size = 1000;
        }
        else{
            packet.size = filelen;
        }
        filelen -= 1000;
        fread(packet.filedata,packet.size,1,fileptr); //fread increments fileptr

        char packetString[1024];
        sprintf(packetString, "%u", packet.total_frag);
        strcat(packetString, ":");
        sprintf(packetString + strlen(packetString), "%u", packet.frag_no);
        strcat(packetString, ":");
        sprintf(packetString + strlen(packetString), "%u",packet.size);
        strcat(packetString, ":");
        sprintf(packetString + strlen(packetString), "%s",packet.filename);
        strcat(packetString, ":");
        int packetHeaderLen = strlen(packetString);
        memcpy(packetString + strlen(packetString), packet.filedata, packet.size);
        packetString[strlen(packetString)] = '\0';
        puts (packetString);
        if ((sendNumBytes = sendto(sockfd, packetString, packetHeaderLen + packet.size, 0,
            servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
            perror("deliver: sendto");
            exit(1);
        }
        recieveNumBytes = recvfrom(sockfd, (char *)buffer, 1000,  
                    0, servinfo->ai_addr, 
                    &servinfo->ai_addrlen);
        buffer[recieveNumBytes] = '\0';
        if (strcmp(buffer, "ACK") == 0){
            printf("Navid is Gae\n");
        } 
    }
    
    
    
    // start = clock();
    // if ((sendNumBytes = sendto(sockfd, proto, strlen(proto), 0,
    //     servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
    //     perror("deliver: sendto");
    //     exit(1);
    // }

    // recieveNumBytes = recvfrom(sockfd, (char *)buffer, 1000,  
    //             0, servinfo->ai_addr, 
    //             &servinfo->ai_addrlen);
    // end = clock();
    // cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    // printf("RTT IS %f seconds\n", cpu_time_used);
    // buffer[recieveNumBytes] = '\0';
    // if (strcmp(buffer, "yes") == 0){
    //     printf("A file transfer can start\n");
    // } 


    //printf("Server : %s\n", buffer);

    freeaddrinfo(servinfo);
    //printf("deliver: sent %d bytes to server\n", sendNumBytes);
    close(sockfd);

    return 0;
}
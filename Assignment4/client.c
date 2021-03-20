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
#include "message.h"




int recvtimeout(int s, char *buf, int len, struct addrinfo * servinfo, int timeout)
{
    fd_set fds;
    int n;
    struct timeval tv;
    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);
    // set up the struct timeval for the timeout
    tv.tv_sec = 0;
    tv.tv_usec = timeout;
    // wait until timeout or data received
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error
    // data must be here, so do a normal recv()
    return recvfrom(s, (char *)buf, len,  
                    0, servinfo->ai_addr, 
                    &servinfo->ai_addrlen);
}

int checkCommand(char * command){
    if (strcmp(command, "/login") == 0){
        return LOGIN;
    }
    else if (strcmp(command, "/logout") == 0){
        return EXIT;
    }
    else if (strcmp(command, "/joinsession") == 0){
        return JOIN;
    }
    else if (strcmp(command, "/leavesession") == 0){
        return LEAVE_SESS;
    }
    else if(strcmp(command, "/createsession") == 0){
        return NEW_SESS;
    }
    else if(strcmp(command, "/list") == 0){
        return QUERY;
    }
    else if(strcmp(command, "/quit") == 0){
        return -1;
    }
    else{
        return MESSAGE;
    }
}

void commandAction(int command, char * theRest){


    if (command == -1){

    }

}

int main( int argc, char *argv[] )
{
    clock_t start, end;
    double cpu_time_used;
    char * serverAddress;
    char * serverPortNum;
    char inputPre[500];
    struct Message message;

    int quit = 0;
    
    while(!quit){
        fgets(inputPre, sizeof(inputPre), stdin);
        char * inputPost = strtok(inputPre, "\n");
        char * firstWord = strtok(inputPost, " ");
        int command = checkCommand(firstWord);
        commandAction(clock)

        
        if (command == NULL){
            exit(1);
        }

    }
    
    char * fileName = strtok(NULL, " ");




    FILE *fileptr;
    long filelen;
    fileptr = fopen(packet.filename,"rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);
    


    int sockfd;
    struct addrinfo hints, *servinfo;
    int rv;
    int sendNumBytes;
    int recieveNumBytes;
    char buffer[1000];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
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
    //, timeOut, resend, timeOutTime, devRTT, estimatedRTT
    int G = 1000;
    char packetString[1024];
    // timeOutTime = 10000;
    // for (x = 1; x <= packet.total_frag; x++){
    //     packet.frag_no = (unsigned int)x;
    //     if (filelen > 1000){
    //         packet.size = 1000;
    //     }
    //     else{
    //         packet.size = (unsigned int)filelen;
    //     }
    //     filelen -= 1000;
    //     fread(packet.filedata, 1, packet.size,fileptr); //fread increments fileptr
    //     memset(packetString,0,sizeof(packetString));

    //     sprintf(packetString, "%u", packet.total_frag);
    //     strcat(packetString, ":");
    //     sprintf(packetString + strlen(packetString), "%u", packet.frag_no);
    //     strcat(packetString, ":");
    //     sprintf(packetString + strlen(packetString), "%u",packet.size);
    //     strcat(packetString, ":");
    //     sprintf(packetString + strlen(packetString), "%s",packet.filename);
    //     strcat(packetString, ":");
    //     int packetHeaderLen = strlen(packetString);
    //     memcpy(packetString + strlen(packetString), packet.filedata, packet.size);
    //     start = clock();
    //     if ((sendNumBytes = sendto(sockfd, packetString, packetHeaderLen + packet.size, 0,
    //         servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
    //         perror("deliver: sendto1");
    //         exit(1);
    //     }
    //     resend = 0;
    //     do { 
    //         timeOut = recvtimeout(sockfd, buffer, 1000, servinfo, timeOutTime);
    //         if (timeOut == -2){
    //             if (resend == 3){
    //                 perror("TIMEOUT");
    //                 exit(1);
    //             }
    //             if ((sendNumBytes = sendto(sockfd, packetString, packetHeaderLen + packet.size, 0,
    //                 servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
    //                 perror("deliver: sendto2");
    //                 exit(1);
    //             }
    //             puts("Attempting to resend");
    //             resend ++;
                
    //         }
    //         else if (timeOut == -1){
    //             perror("recev: fram");
    //             exit(1);
    //         }
    //     }while(timeOut == -2);
    //     resend = 0;
    //     end = clock();
    //     cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    //     if (x == 1){
    //         estimatedRTT = cpu_time_used * 1000000;
    //         devRTT = cpu_time_used*1000000/2;
    //         timeOutTime = estimatedRTT + fmax(4*devRTT, G);
    //     }
    //     else{
    //         estimatedRTT = (int)((1-0.125)*estimatedRTT + 0.125*(cpu_time_used * 1000000));
    //         devRTT = (int)(0.75*devRTT + 0.25*abs(cpu_time_used * 1000000 - estimatedRTT));
    //         timeOutTime = estimatedRTT + fmax(4*devRTT, G);
    //     }

        
    // }

    if ((sendNumBytes = sendto(sockfd, packetString, packetHeaderLen + packet.size, 0,
        servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
        perror("deliver: sendto1");
        exit(1);
    }

    buffer[recieveNumBytes] = '\0';
    if (strcmp(buffer, "ACK") == 0){
        printf("ACK received\n");
    }
    
    

    freeaddrinfo(servinfo);
    close(sockfd);
    fclose(fileptr);
    return 0;
}
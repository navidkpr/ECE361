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

char serverAddress[100];
char serverPortNum[100];
int loggedIn, inSession;


// int recvtimeout(int s, char *buf, int len, struct addrinfo * servinfo, int timeout)
// {
//     fd_set fds;
//     int n;
//     struct timeval tv;
//     // set up the file descriptor set
//     FD_ZERO(&fds);
//     FD_SET(s, &fds);
//     // set up the struct timeval for the timeout
//     tv.tv_sec = 0;
//     tv.tv_usec = timeout;
//     // wait until timeout or data received
//     n = select(s+1, &fds, NULL, NULL, &tv);
//     if (n == 0) return -2; // timeout!
//     if (n == -1) return -1; // error
//     // data must be here, so do a normal recv()
//     return recvfrom(s, (char *)buf, len,  
//                     0, servinfo->ai_addr, 
//                     &servinfo->ai_addrlen);
// }

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



//return 0 on success
int messagePopulate(int command,char * theFirst, char * theRest, struct Message * message){
    int dataLen = 0;
    if(command == LOGIN){
        message->type = LOGIN;
        char * cID = strtok(NULL, " ");
        char * pass = strtok(NULL, " ");
        char * servIP = strtok(NULL, " ");
        char * servPort = strtok(NULL, " ");
        if (cID == NULL || pass == NULL || servIP == NULL || servPort == NULL){
            return 1;
        }
        strcpy(message->source, cID);
        strcpy(serverAddress, servIP);
        strcpy(serverPortNum, servPort);
        message->size = strlen(pass);
        strcpy(message->data, pass);
    }
    else if(command == EXIT || (loggedIn && command == -1)){ //exit server
        message->type = EXIT;
        message->size = 0;
        strcpy(message->data, "");
    }
    else if(command == JOIN){
        message->type = JOIN;
        if (theRest == NULL){
            return 1;
        }
        dataLen = strlen(theRest);
        message->size = dataLen;
        strcpy(message->data, theRest);
    }
    else if(command == NEW_SESS){
        message->type = NEW_SESS;
        if (theRest == NULL){
            return 1;
        }
        dataLen = strlen(theRest);
        message->size = dataLen;
        strcpy(message->data, theRest);
    }
    else if(command == LEAVE_SESS){
        message->type = LEAVE_SESS;
        message->size = 0;
        strcpy(message->data, "");
        inSession = 0; //this may not want to be here
    }
    else if(command == QUERY){ //list
        message->type = QUERY;
        message->size = 0;
        strcpy(message->data, "");

    }
    else if(command == -1){ //quit
        message->type = -1;
    }
    else if(inSession){
        message->type = MESSAGE;
        dataLen += sprintf(message->data, "%s %s",theFirst, theRest);
        message->size = dataLen; //excluding NULL character
    }
    else{
        return 2;
    }

    return 0;
}

void printAckAndUpdateSession(struct Message * resp){
    if (resp->type == LO_ACK){
        puts("We IN\n");
    }
    else if(resp->type == LO_NACK){
        printf("%s\n", resp->data);
    }
    else if(resp->type == JN_ACK){
        puts("We Joined\n");
        inSession = 1;
    }
    else if(resp->type == JN_NACK){
        printf("%s\n", resp->data);
    }
    else if(resp->type == NS_ACK){
        puts("We Created a Sesh\n");
        inSession = 1;
    }
    else if(resp->type == NS_NACK){
        printf("%s\n", resp->data);
    }
    else if(resp->type == QU_ACK){
        printf("Session List: \n %s", resp->data);
    }
    else if(resp->type == LS_ACK){
        puts("We Outta Session\n");
        inSession = 0;
    }
}

int main( int argc, char *argv[] )
{
    loggedIn = 0;
    inSession = 0;
    int quit = 0;

    int sockfd;
    struct addrinfo hints, *servinfo;
    int rv;
    int sendNumBytes;
    int recieveNumBytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE;

    fd_set read_fds;

    char buffer[1000];
    
    while(!quit){
        int err;
        struct Message message;
        char inputPre[500];

        if (inSession){
            int fd_max = STDIN_FILENO;

            /* Set the bits for the file descriptors you want to wait on. */
            FD_ZERO(&read_fds);
            FD_SET(STDIN_FILENO, &read_fds);
            FD_SET(sockfd, &read_fds);

            if( sockfd > fd_max ) { fd_max = sockfd; }

            if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1)
            {
                perror("select:");
                exit(1);
            }
            if( FD_ISSET(sockfd, &read_fds) )
            {
                /* There is data waiting on your socket.  Read it with recv(). */
                if ((recieveNumBytes = recv(sockfd, (char *)buffer, 1000, 0)) == -1){
                    perror("client: recv1");
                    exit(1);
                }
                buffer[recieveNumBytes] = '\0';
                struct Message recvMsg;
                parse_message(buffer, &recvMsg);
                printf("From Conf Session: %s\n", recvMsg.data);
            }

            if( FD_ISSET(STDIN_FILENO, &read_fds ))
            {
                /* The user typed something.  Read it fgets or something.
                Then send the data to the server. */
                fgets(inputPre, sizeof(inputPre), stdin);
                char * inputPost = strtok(inputPre, "\n");
                char * firstWord = strtok(inputPost, " ");
                inputPost = inputPost + strlen(inputPost) + 1; //to find theRest
                
                int command = checkCommand(firstWord);
                if (command == -1 && !loggedIn){
                    break;
                }
                else if (command == -1 && loggedIn){
                    quit = 1;
                }

                
                err = messagePopulate(command, firstWord, inputPost, &message);
                if (err){
                    if (err == 1){
                        puts("BRODY, U DONE GOOFED WIT DA COMMAND, TRY AGAIN\n");
                    }
                    else if (err == 2){
                        puts("class is not in SESSION, u can't send dis\n");
                    }
                    
                    continue;
                }

                //-----------------------------------------------------------------------------------------------------------

                if (!loggedIn && command == LOGIN){
                    if ((rv = getaddrinfo(serverAddress, serverPortNum, &hints, &servinfo)) != 0) {
                        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                        return 1;
                    }
                    
                    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
                    if(sockfd < 0){
                        perror("Error while creating socket\n");
                        return -1;
                    }
                    printf("Socket created successfully\n");

                    if(connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
                        close(sockfd);
                        perror("Error connecting to socket\n");
                    }
                    printf("Connection successful\n");
                    loggedIn = 1;
                }
                

                
                
                char messageString[MAX_OVER_NETWORK];
                memset(messageString,0,sizeof(messageString));
                sprintf(messageString, "%d:%d:%s:%s", message.type, message.size, message.source, message.data);

                if ((sendNumBytes = send(sockfd, messageString, strlen(messageString), 0) == -1)) {
                    perror("client: send1");
                    exit(1);
                }
                //printf("Sent: %s\n", messageString);

                if(loggedIn && command == EXIT){
                    freeaddrinfo(servinfo);
                    close(sockfd);
                    loggedIn = 0;
                    inSession = 0;
                    continue;
                }

                char buffer[1000];

                if ((recieveNumBytes = recv(sockfd, (char *)buffer, 1000, 0)) == -1){
                    perror("client: recv1");
                    exit(1);
                }
                buffer[recieveNumBytes] = '\0';
                //printf("Received: %s\n", buffer);

                struct Message recvMsg;
                parse_message(buffer, &recvMsg);

                if (recvMsg.type == LO_NACK){ //would i close something already closed??
                    freeaddrinfo(servinfo);
                    close(sockfd);
                    loggedIn = 0;
                }

                printAckAndUpdateSession(&recvMsg);

                
            }
        }
        else{
            fgets(inputPre, sizeof(inputPre), stdin);
            char * inputPost = strtok(inputPre, "\n");
            char * firstWord = strtok(inputPost, " ");
            inputPost = inputPost + strlen(inputPost) + 1; //to find theRest
            
            int command = checkCommand(firstWord);
            if (command == -1 && !loggedIn){
                break;
            }
            else if (command == -1 && loggedIn){
                quit = 1;
            }

            
            err = messagePopulate(command, firstWord, inputPost, &message);
            if (err){
                if (err == 1){
                    puts("BRODY, U DONE GOOFED WIT DA COMMAND, TRY AGAIN\n");
                }
                else if (err == 2){
                    puts("class is not in SESSION, u can't send dis\n");
                }
                
                continue;
            }

            //-----------------------------------------------------------------------------------------------------------

            if (!loggedIn && command == LOGIN){
                if ((rv = getaddrinfo(serverAddress, serverPortNum, &hints, &servinfo)) != 0) {
                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                    return 1;
                }
                
                sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
                if(sockfd < 0){
                    perror("Error while creating socket\n");
                    return -1;
                }
                printf("Socket created successfully\n");

                if(connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
                    close(sockfd);
                    perror("Error connecting to socket\n");
                }
                printf("Connection successful\n");
                loggedIn = 1;
            }
            

            
            
            char messageString[MAX_OVER_NETWORK];
            memset(messageString,0,sizeof(messageString));
            sprintf(messageString, "%d:%d:%s:%s", message.type, message.size, message.source, message.data);

            if ((sendNumBytes = send(sockfd, messageString, strlen(messageString), 0) == -1)) {
                perror("client: send1");
                exit(1);
            }
            //printf("Sent: %s\n", messageString);

            if(loggedIn && command == EXIT){
                freeaddrinfo(servinfo);
                close(sockfd);
                loggedIn = 0;
                inSession = 0;
                continue;
            }

            if ((recieveNumBytes = recv(sockfd, (char *)buffer, 1000, 0)) == -1){
                perror("client: recv1");
                exit(1);
            }
            buffer[recieveNumBytes] = '\0';
            //printf("Received: %s\n", buffer);

            struct Message recvMsg;
            parse_message(buffer, &recvMsg);

            if (recvMsg.type == LO_NACK){ //would i close something already closed??
                freeaddrinfo(servinfo);
                close(sockfd);
                loggedIn = 0;
            }

            printAckAndUpdateSession(&recvMsg);
        }

    }

    
    return 0;
}

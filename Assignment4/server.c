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
#define MYHOST "ug137"

#include "message.h"

FILE * fPtr;
const char* users[] = {"Nathan, Robert, Navid, YourMom, Hamid"};
const char* pwds[] = {"red, orange, yellow, green, blue"};

struct Message * parse_message(char *str) {
    
    int type = 0;
    int size = 0;

    struct Message* msg;

    int pt = 0;
    while (str[pt] != ':') {
        type = type * 10 + str[pt] - '0';
        pt++;
    }
    msg->type = type;

    pt++;
    while (str[pt] != ':') {
        size = size * 10 + str[pt] - '0';
        pt++;
    }
    msg->size = size;

    pt++;
    int st_pt = pt;
    while (str[pt] != ':')
        pt++;

    char source[sizeof(char) * (pt - st_pt + 1)];
    memcpy (source, &str[st_pt], pt - st_pt);
    source[pt - st_pt] = '\0';
    memcpy(msg->source, source, pt - st_pt + 1);

    pt++;
    st_pt = pt;
    
    char data[sizeof(char) * (size + 1)];
    memcpy (data, &str[st_pt], size);
    data[size] = '\0';
    memcpy(msg->data, data, size + 1);

    return msg;
}

void command_handler(struct Message* msg, int client_fd){
    int type = msg->type;
    char* ack_msg;
    char* source = "Server";
    bool authorized = false;
    if(type == LOGIN){
        for(int i = 0; i < len(users); i++){
            if(strcmp(users[i], msg->source) && strcmp(pwds[i], msg->data)){
                char* data = " ";
                sprintf(ack_msg, "%u:%u:%s:%s", LO_ACK, len(data), source, data);
                send(client_fd, ack_msg, len(ack_msg), 0);
                authorized = true;
            }
        }
        if(!authorized){
            char* data = "Incorrect login info didiot";
            sprintf(ack_msg, "%u:%u:%s:%s", LO_NACK, len(data), source, data);
        }
    }else if(type == EXIT){
        //remove socket from data structure
        close(client_fd);
    }else if(type == JOIN){
        //add socket to session data structure
    }else if(type == LEAVE_SESS){
        //remove socket from session data structure
    }else if(type == NEW_SESS){
        //create session data structure 
        //add socket to session data structure
    }else if(type == QUERY){
        //send a message of all online users and available sessions
    }else if(type == MESSAGE){
        //loop through sockets in specified conference session sending the message
        send(curr_fd, "%u:%u:%s:%s", MESSAGE, len(msg->data), source, msg->data);
    }else if(type == QUIT){
        //remove socket from session data structure
        close(client_fd);
    }
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
    int yes = 1;
    fd_set sock_set;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(MYHOST, serverPortNum, &hints, &res);

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
        printf("Error setting socket options\n");
        return -1;
    }
    
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        printf("Couldn't bind to the port\n");
        return -1;
    }

    printf("Done with binding\n");

    if(listen(sockfd, BACKLOG) == -1){
        printf("Error while trying to listen for incoming connections\n");
        return -1;
    }

    printf("Listening for incoming messages...\n\n");

    int isDone = 0;
    char message_str[MAX_DATA];
    int rec_num_bytes = 0;

    while (!isDone) {

        addr_size = sizeof client_addr;
        char packet[1050];
        new_fd = accept(sockfd, (struct sockaddr *) &client_addr, &addr_size);
        if (new_fd == -1){
            printf("Error accepting new connection\n");
            return -1;
        }

        rec_num_bytes = recv(new_fd, message_str, MAX_DATA-1, 0);
        message_str[rec_num_bytes] = '\0';

        struct Message* msg;
        msg = parse_message(message_str);


        if(send(new_fd, "Your Mom!", 9, 0) == -1){
            printf("Error sending message\n");
            return -1;
        }
        break;
    }

    close(sockfd);
}
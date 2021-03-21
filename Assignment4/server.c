#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>

#define BACKLOG 10 
#define MYHOST "ug136"
#define NUM_USERS 5

#include "message.h"


struct Session {
    char *id;
    struct Session *next;
}; 

FILE * fPtr;
const char* users[] = {"Nathan", "Robert", "Navid", "YourMom", "Hamid"};
const char* pwds[] = {"red", "orange", "yellow", "green", "blue"};
struct Session *sessions[NUM_USERS] = {NULL, NULL, NULL, NULL};
struct Session *head = NULL;
int is_active[] = {1, 1, 1, 1, 1};

void parse_message(char *str, struct Message* msg) {
    
    int type = 0;
    int size = 0;

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
}

void command_handler(struct Message* msg, int client_fd){
    int type = msg->type;
    char ack_msg[MAX_OVER_NETWORK];
    memset(ack_msg,0,sizeof(ack_msg));
    char* source = "Server";
    bool authorized = false;
    int i;
    if(type == LOGIN){
        for(int i = 0; i < 5; i++){
            // printf("user: %s\n", users[i]);
            // printf("source: %s\n", msg->source);
            if(!strcmp(users[i], (char*)msg->source) && !strcmp(pwds[i], (char*)msg->data)){
                char* data = " ";
                sprintf(ack_msg, "%d:%d:%s:%s", LO_ACK, strlen(data), source, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
                authorized = true;
                break;
            }
        }
        if(!authorized){
            char* data = "Incorrect login info didiot";
            sprintf(ack_msg, "%d:%d:%s:%s", LO_NACK, strlen(data), source, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
        }
    }else if(type == EXIT){
        //remove client socket from data structure
        char *client_id = msg->source;
        for (int i = 0; i < NUM_USERS; i++)
            if (strcmp(users[i], client_id) == 0) {
                is_active[i] = 0;
                sessions[i] = NULL;
            }
        close(client_fd);
    }else if(type == JOIN){
        char *session_id = msg->data;
        struct Session *cur = head;
        int isFound = 0;
        while (cur != NULL) {
            if (strcmp(session_id, cur->id) == 0) {
                sprintf(ack_msg, "Joined session %s\n", session_id);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
                isFound = 1;
            }
            cur = cur->next;
        }
        if (!isFound) {
            sprintf(ack_msg, "Session %s not found\n", session_id);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
        }

    }else if(type == LEAVE_SESS){
        char *client_id = msg->source;
        for (i = 0; i < NUM_USERS; i++)
            if (strcmp(client_id, users[i]) == 0)
                sessions[i] = NULL;
        send(client_fd, ack_msg, strlen(ack_msg), 0);
        //remove socket from session data structure
        ;
    }else if(type == NEW_SESS){
        char *session_id = msg->data;

        struct Session *pre = NULL;
        struct Session *cur = head;
         
        while (cur != NULL) {
            if (strcmp(session_id, cur->id) == 0) {
                sprintf(ack_msg, "Session %s already exists\n", session_id);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
            }
            pre = cur;
        }
        pre->next = malloc(sizeof(struct Session));
        pre->next->id = session_id;
        
        //create session data structure 
        //add socket to session data structure
        ;
    }else if(type == QUERY){
        for (i = 0; i < NUM_USERS; i++)
            if (is_active[i]){
                if(sessions[i] == NULL){
                    sprintf(ack_msg + strlen(ack_msg), "%s:No Session\n", users[i]);
                }else{
                    sprintf(ack_msg + strlen(ack_msg), "%s:%s\n", users[i], sessions[i]->id);
                }
            }
        send(client_fd, ack_msg, strlen(ack_msg), 0);
        //send a message of all online users and available sessions
        ;
    }else if(type == MESSAGE){
        //loop through sockets in specified conference session sending the message
        //TODO: Need to check if in a session
        char *client_id = msg->source;
        struct Session *cur_session = NULL;
        for (i = 0; i < NUM_USERS; i++)
            if (is_active[i] && strcmp(users[i], client_id) == 0)
                cur_session = sessions[i];
        for (i = 0; i < NUM_USERS; i++)
            if (is_active[i] && sessions[i] == cur_session) {
                //send the message to this client
            } 
        sprintf(ack_msg, "%d:%d:%s:%s", MESSAGE, strlen(msg->data), source, msg->data);
        send(client_fd, ack_msg, strlen(ack_msg), 0);
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
    char message_str[MAX_OVER_NETWORK];
    int rec_num_bytes = 0;
    addr_size = sizeof client_addr;

    new_fd = accept(sockfd, (struct sockaddr *) &client_addr, &addr_size);
    if(new_fd  < 0){
        perror("Error accepting new connection:");
        return -1;
    }
    printf("Connection accepted\n");

    while (!isDone) {
        
        // FD_ZERO(&sock_set);
        // FD_SET(sockfd, &sock_set);
        // int max_sd = sockfd;

        // for(int i = 0; i < 10/*MAX_CLIENTS*/; i++){
        //     //sd = socket_fd//whatever the socket data structure is 
        //     int sd = 0;
        //     if(sd > 0)
        //         FD_SET(sd, &sock_set);
            
        //     if(sd > max_sd)
        //         max_sd = sd;
        // }

        // if(select(max_sd+1, &sock_set, NULL, NULL, NULL) < 0){
        //     printf("Error selecting socket\n");
        //     return -1;
        // }
        // printf("Socket selected\n");

        // if(FD_ISSET(sockfd, &sock_set)){
        //     if(new_fd = accept(sockfd, (struct sockaddr *) &client_addr, &addr_size) < 0){
        //         printf("Error accepting new connection\n");
        //         return -1;
        //     }
        //     printf("Connection accepted\n");
        // }
        // new_fd = accept(sockfd, (struct sockaddr *) &client_addr, &addr_size);
        // if(new_fd  < 0){
        //     perror("Error accepting new connection:");
        //     return -1;
        // }
        // printf("Connection accepted\n");

        rec_num_bytes = recv(new_fd, message_str, MAX_OVER_NETWORK, 0);
        if(rec_num_bytes == -1){
            perror("Error receiving message:");
            return -1;
        }
        message_str[rec_num_bytes] = '\0';

        struct Message msg;
        parse_message(message_str, &msg);

        command_handler(&msg, new_fd);

    }

    close(sockfd);
}

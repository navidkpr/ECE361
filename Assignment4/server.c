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
#define NUM_USERS 5

#include "message.h"

char MYHOST[10];
struct Session {
    char *id;
    struct Session *next;
}; 

FILE * fPtr;
const char* users[] = {"Nathan", "Robert", "Navid", "YourMom", "Hamid"};
const char* pwds[] = {"red", "orange", "yellow", "green", "blue"};
struct Session *sessions[NUM_USERS] = {NULL, NULL, NULL, NULL, NULL};
int client_fds[NUM_USERS] = {-1, -1, -1, -1, -1};
struct Session *head = NULL;
int is_active[] = {0, 0, 0, 0, 0};

void command_handler(struct Message* msg, int client_fd){
    int type = msg->type;
    char ack_msg[MAX_OVER_NETWORK];
    memset(ack_msg,0,sizeof(ack_msg));
    char* source = "Server";
    bool authorized = false;
    int i;
    if(type == LOGIN){
        for(int i = 0; i < NUM_USERS; i++){
            // printf("user: %s\n", users[i]);
            // printf("source: %s\n", msg->source);
            if(!strcmp(users[i], (char*)msg->source) && !strcmp(pwds[i], (char*)msg->data)){
                client_fds[i] = client_fd;
                char* data = " ";
                sprintf(ack_msg, "%d:%d:%s:%s", LO_ACK, strlen(data), source, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
                authorized = true;
                is_active[i] = 1;
                break;
            }
        }
        if(!authorized){
            char* data = "Incorrect login info didiot";
            sprintf(ack_msg, "%d:%d:%s:%s", LO_NACK, strlen(data), source, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            close(client_fd);
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
        char *client_id = msg->source;

        struct Session *cur = head;
        int isFound = 0;
        while (cur != NULL) {
            if (strcmp(session_id, cur->id) == 0) {
                for (i = 0; i < NUM_USERS; i++)
                    if (strcmp(users[i], client_id) == 0)
                        sessions[i] = cur;

                isFound = 1;
                char data[MAX_OVER_NETWORK];
                memset(data, 0, sizeof(data));
                sprintf(data, "Session %s joined", session_id);
                sprintf(ack_msg, "%d:%d:%s:%s", JN_ACK, strlen(data), session_id, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
                printf("Session joined\n");

            }
            cur = cur->next;
        }
        if (!isFound) {
            char data[MAX_OVER_NETWORK];
            memset(data, 0, sizeof(data));
            sprintf(data, "Session %s not found", session_id);
            sprintf(ack_msg, "%d:%d:%s:%s", JN_NACK, strlen(data), session_id, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            printf("Session not found\n");

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
        //puts("We made it boise\n");
        char *session_id = (char *)msg->data;
        char *client_id = (char *)msg->source;

        struct Session *pre = NULL;
        struct Session *cur = head;

        //puts("check - 2\n"); 
        while (cur != NULL) {
            if (strcmp(session_id, cur->id) == 0) {
                char data[MAX_OVER_NETWORK];
                memset(data, 0, sizeof(data));
                sprintf(data, "Session %s already exists", session_id);
                sprintf(ack_msg, "%d:%d:%s:%s", NS_NACK, strlen(data), session_id, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
            }
            pre = cur;
            cur = cur->next;
        }
        //puts("check - 3\n");
        if (head == NULL){
            head = malloc(sizeof(struct Session));
            strcpy(head->id, session_id);
            head->next = NULL;
            
            for (i = 0; i < NUM_USERS; i++)
                if (strcmp(client_id, users[i]) == 0)
                    sessions[i] = head;
        }
        else{
            pre->next = malloc(sizeof(struct Session));
            strcpy(pre->next->id, session_id);
            for (i = 0; i < NUM_USERS; i++)
                if (strcmp(client_id, users[i]) == 0)
                    sessions[i] = pre->next;

            pre->next->next = NULL;
        }

        char data[MAX_OVER_NETWORK];
        memset(data, 0, sizeof(data));
        sprintf(data, "Session %s created", session_id);
        sprintf(ack_msg, "%d:%d:%s:%s", NS_ACK, strlen(data), session_id, data);
        send(client_fd, ack_msg, strlen(ack_msg), 0);
        printf("Session created\n");
        //create session data structure 
        //add socket to session data structure
    }else if(type == QUERY){
        char data[MAX_OVER_NETWORK];
        memset(data, 0, sizeof(data));
        for (i = 0; i < NUM_USERS; i++){ 
            if (is_active[i]){
                if(sessions[i] == NULL){
                    sprintf(data + strlen(data), "%s:No Session\n", users[i]);
                }else{
                    sprintf(data + strlen(data), "%s:%s\n", users[i], sessions[i]->id);
                }
            }
        }
        sprintf(ack_msg, "%d:%d:%s:%s", QU_ACK, strlen(data), msg->source, data);
        send(client_fd, ack_msg, strlen(ack_msg), 0);
        //send a message of all online users and available sessions
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
                sprintf(ack_msg, "%d:%d:%s:%s", MESSAGE, strlen(msg->data), source, msg->data);
                send(client_fds[i], ack_msg, strlen(ack_msg), 0);
                //send the message to this client
            } 
    }
}

int main( int argc, char *argv[] ) {
    if (argc != 2){
        fprintf(stderr,"usage: server ServerPortNumber\n");
        exit(1);
    }
    char * serverPortNum = argv[1];

    gethostname(MYHOST, 10);

    struct sockaddr_storage client_addr;
    
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int serv_fd, new_fd;
    int recieveNumBytes;
    int yes = 1;
    fd_set sock_set;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(MYHOST, serverPortNum, &hints, &res);

    serv_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(serv_fd < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    if(setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
        printf("Error setting socket options\n");
        return -1;
    }
    
    if (bind(serv_fd, res->ai_addr, res->ai_addrlen) < 0) {
        printf("Couldn't bind to the port\n");
        return -1;
    }

    printf("Done with binding\n");

    if(listen(serv_fd, BACKLOG) == -1){
        printf("Error while trying to listen for incoming connections\n");
        return -1;
    }

    printf("Listening for incoming messages...\n\n");

    //FD_SET(serv_fd, &master);

    int isDone = 0;
    char message_str[MAX_OVER_NETWORK];
    int rec_num_bytes = 0;
    addr_size = sizeof client_addr;
    int max_sd = serv_fd;

    while (!isDone) {
        
        FD_ZERO(&sock_set);
        FD_SET(serv_fd, &sock_set);


        for(int i = 0; i < NUM_USERS; i++){ 
            if(is_active[i]){ 
                int s_fd = client_fds[i];
                if(s_fd > 0)
                    FD_SET(s_fd, &sock_set);
                
                if(s_fd > max_sd)
                    max_sd = s_fd;
            }
        }

        if(select(max_sd+1, &sock_set, NULL, NULL, NULL) < 0){
            perror("Error selecting socket:");
            return -1;
        }
        //printf("Socket selected\n");

        if(FD_ISSET(serv_fd, &sock_set)){
            new_fd = accept(serv_fd, (struct sockaddr *) &client_addr, &addr_size);
            if(new_fd < 0){
                perror("Error accepting new connection:");
                return -1;
            }
            printf("Connection accepted\n");

            rec_num_bytes = recv(new_fd, message_str, MAX_OVER_NETWORK, 0);
            if(rec_num_bytes == -1){
                perror("Error receiving message:");
                return -1;
            }
            printf("Message received \n");

            message_str[rec_num_bytes] = '\0';

            struct Message msg;
            parse_message(message_str, &msg);

            command_handler(&msg, new_fd);
        }else{

            for(int i = 0; i < NUM_USERS; i++){
                int s_fd = client_fds[i];

                if(FD_ISSET(s_fd, &sock_set)){
                    printf("%d\n",s_fd);
                    rec_num_bytes = recv(s_fd, message_str, MAX_OVER_NETWORK, 0);
                
                    if(rec_num_bytes == -1){
                        perror("Error receiving message:");
                        return -1;
                    }

                    message_str[rec_num_bytes] = '\0';

                    struct Message msg;
                    parse_message(message_str, &msg);

                    command_handler(&msg, s_fd);
                }
                
            }
            
        }
    }

    close(serv_fd);
}

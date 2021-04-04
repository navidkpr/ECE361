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

#define MAX_ID 100
#define MAX_HOST 10
#define BACKLOG 10 
#define NUM_USERS 5
#define MAX_SESSIONS 3

#include "message.h"

char MYHOST[MAX_HOST];
struct Session {
    char id[MAX_ID];
    int count;
    //struct Session *next;
}; 

struct Client {
    char user_id[MAX_ID];
    int fd;
    bool is_active;
    bool has_sessions;
    struct Session *sessions[MAX_SESSIONS];
};

struct Client *clients[NUM_USERS] = {NULL, NULL, NULL, NULL, NULL};


FILE * fPtr;
const char* users[] = {"u1", "u2", "Navid", "YourMom", "Hamid"};
const char* pwds[] = {"p1", "p2", "yellow", "green", "blue"};
//struct Session *sessions[NUM_USERS] = {NULL, NULL, NULL, NULL, NULL};
//int client_fds[NUM_USERS] = {-1, -1, -1, -1, -1};
//struct Session *head = NULL;
//int is_active[] = {0, 0, 0, 0, 0};

void command_handler(struct Message* msg, int client_fd){
    int type = msg->type;
    char ack_msg[MAX_OVER_NETWORK];
    memset(ack_msg,0,sizeof(ack_msg));
    char* source = "Server";
    char from[100];
    bool authorized = false;
    int i;
    if(type == LOGIN){
        for(i = 0; i < NUM_USERS; i++){
            // printf("user: %s\n", users[i]);
            // printf("source: %s\n", msg->source);
            if((strcmp(users[i], (char*)msg->source)==0) && (strcmp(pwds[i], (char*)msg->data)==0)){
                if(clients[i] == NULL || !clients[i]->is_active){
                    if(clients[i] == NULL){ 
                        struct Client *to_add;
                        to_add = malloc(sizeof(struct Client));
                        to_add->fd = client_fd;
                        strcpy(to_add->user_id, users[i]);
                        to_add->is_active = true;
                        to_add->has_sessions = false;
                        for(int j = 0; j < MAX_SESSIONS; j++){
                            to_add->sessions[j] = NULL;
                        }
                        // to_add->session = NULL;
                        clients[i] = to_add;
                    }else{
                        clients[i]->fd = client_fd;
                        clients[i]->is_active = true;
                    }
                    char* data = " ";
                    sprintf(ack_msg, "%d:%d:%s:%s", LO_ACK, strlen(data), source, data);
                    send(client_fd, ack_msg, strlen(ack_msg), 0);
                    authorized = true;
                    break;
                }else if(clients[i]->is_active){
                    char* data = "Already logged in";
                    sprintf(ack_msg, "%d:%d:%s:%s", LO_NACK, strlen(data), msg->source, data);
                    send(client_fd, ack_msg, strlen(ack_msg), 0);
                    authorized = true;
                    break;
                }
            }
            if(authorized) break;
        }
        if(!authorized && (clients[i] == NULL || !clients[i]->is_active)){
            char* data = "Incorrect login info didiot";
            sprintf(ack_msg, "%d:%d:%s:%s", LO_NACK, strlen(data), source, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            close(client_fd);
        }
    }else if(type == EXIT){
        //remove client socket from data structure
        char *client_id = msg->source;
        for (int i = 0; i < NUM_USERS; i++){
            if (strcmp(users[i], client_id) == 0) {
                clients[i]->is_active = false;
                if(clients[i]->has_sessions){
                    for(int j = 0; j < MAX_SESSIONS; j++){
                        if(clients[i]->sessions[j] != NULL){
                            clients[i]->sessions[j] = NULL;
                        }
                    }
                }
                // free(clients[i]);
                // clients[i] = NULL;
            }
        }
        close(client_fd);
    }else if(type == JOIN){
        char *session_id = (char *)msg->data;
        char *client_id = (char *)msg->source;
        bool sess_found = false;

        int i;
        for(i = 0; i < NUM_USERS; i++){
            if(clients[i] != NULL){
                for(int j = 0; j < MAX_SESSIONS; j++){
                    if((clients[i]->has_sessions) && (clients[i]->sessions[j] != NULL && strcmp(session_id, clients[i]->sessions[j]->id) == 0)){
                        sess_found = true;
                        break;
                    }
                }
            }
        }

        if(sess_found){
            struct Session *to_add;
            to_add = malloc(sizeof(struct Session));
            strcpy(to_add->id, session_id);
            for(i = 0; i < NUM_USERS; i++){
                if(strcmp(users[i], client_id) == 0){
                    for(int j = 0; j < MAX_SESSIONS; j++){
                        if(clients[i]->sessions[j] == NULL){
                            clients[i]->sessions[j] = to_add;
                            clients[i]->has_sessions = true;
                            break;
                        }
                    }
                    break;
                }
            }
            char data[MAX_OVER_NETWORK];
            memset(data, 0, sizeof(data));
            sprintf(data, "Session %s joined", session_id);
            sprintf(ack_msg, "%d:%d:%s:%s", JN_ACK, strlen(data), session_id, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            printf("Session joined\n");
        }else{
            char data[MAX_OVER_NETWORK];
            memset(data, 0, sizeof(data));
            sprintf(data, "Session %s not found", session_id);
            sprintf(ack_msg, "%d:%d:%s:%s", JN_NACK, strlen(data), session_id, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            printf("Session not found\n");
        }
    }else if(type == LEAVE_SESS){
        char *client_id = msg->source;
        char *session_id = msg->data;

        for(i = 0; i < NUM_USERS; i++){ //still gotta do what i do
            if(strcmp(users[i], client_id) == 0){
                if(clients[i]->has_sessions){
                    for(int j = 0; j < MAX_SESSIONS; j++){
                        if(clients[i]->sessions[j] != NULL && strcmp(clients[i]->sessions[j]->id, session_id) == 0){
                            clients[i]->sessions[j] = NULL;
                            break;
                        }
                    }
                    char* data = "Left session";
                    sprintf(ack_msg, "%d:%d:%s:%s", LS_ACK, strlen(data), msg->source, data);
                    send(client_fd, ack_msg, strlen(ack_msg), 0);
                    break;
                }
            }
        }
    }else if(type == NEW_SESS){
        //puts("We made it boise\n");
        char *session_id = (char *)msg->data;
        char *client_id = (char *)msg->source;
        int i;
        int j;
        int empty = -1;
        bool sess_found = false;

        struct Session *to_add;
        to_add = malloc(sizeof(struct Session));
        strcpy(to_add->id, session_id);
        to_add->count = 1;

        for(i = 0; i < NUM_USERS; i++){
            if(strcmp(users[i], client_id) == 0){
                if(clients[i] != NULL && clients[i]->has_sessions){
                    for(j = 0; j < MAX_SESSIONS; j++){
                        if(clients[i]->sessions[j] == NULL){
                            if(empty == -1){
                                empty = j;
                            }
                        }else if(strcmp(clients[i]->sessions[j]->id, session_id) == 0){
                            sess_found = true;
                        }
                    }
                    if(!sess_found){
                        if(empty == -1){
                            char data[MAX_OVER_NETWORK];
                            memset(data, 0, sizeof(data));
                            sprintf(data, "Hotboxed\n", session_id);
                            sprintf(ack_msg, "%d:%d:%s:%s", NS_NACK, strlen(data), session_id, data);
                            send(client_fd, ack_msg, strlen(ack_msg), 0);
                            break;
                        }
                        clients[i]->has_sessions = true;
                        clients[i]->sessions[empty] = to_add;
                        char data[MAX_OVER_NETWORK];
                        memset(data, 0, sizeof(data));
                        sprintf(data, "Session %s created", session_id);
                        sprintf(ack_msg, "%d:%d:%s:%s", NS_ACK, strlen(data), session_id, data);
                        send(client_fd, ack_msg, strlen(ack_msg), 0);
                        break;
                    }else if(sess_found){
                        char data[MAX_OVER_NETWORK];
                        memset(data, 0, sizeof(data));
                        sprintf(data, "Session %s already exists", session_id);
                        sprintf(ack_msg, "%d:%d:%s:%s", NS_NACK, strlen(data), session_id, data);
                        send(client_fd, ack_msg, strlen(ack_msg), 0);
                        break;
                    }
                }
                else if(!clients[i]->has_sessions){
                    clients[i]->has_sessions = true;
                    clients[i]->sessions[0] = to_add;
                    char data[MAX_OVER_NETWORK];
                    memset(data, 0, sizeof(data));
                    sprintf(data, "Session %s created", session_id);
                    sprintf(ack_msg, "%d:%d:%s:%s", NS_ACK, strlen(data), session_id, data);
                    send(client_fd, ack_msg, strlen(ack_msg), 0);
                    break;
                }
            }
        }
    }else if(type == QUERY){
        char data[MAX_OVER_NETWORK];
        memset(data, 0, sizeof(data));
        int cnt_sessions = 0;
        for (i = 0; i < NUM_USERS; i++){ 
            if (clients[i] != NULL && clients[i]->is_active){
                cnt_sessions = 0;
                sprintf(data + strlen(data), "%s:", users[i]);
                for(int j = 0; j < MAX_SESSIONS; j++){
                    if(clients[i]->sessions[j] != NULL){
                        cnt_sessions++;
                        sprintf(data + strlen(data), "%s,", clients[i]->sessions[j]->id);
                    }
                }
                if(cnt_sessions == 0){
                    sprintf(data + strlen(data), "No Sessions\n");
                }else{
                    sprintf(data + strlen(data), "\n");
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
        char inputPre[strlen(msg->data)];
        strcpy(inputPre, msg->data);
        char *session_id = strtok(inputPre, " ");
        char *msg_data = inputPre + strlen(inputPre) + 1; //to find theRest

        struct Session *cur_session = NULL;
        bool sess_found = false;
        for (i = 0; i < NUM_USERS; i++){
            if (clients[i] != NULL && clients[i]->is_active && strcmp(users[i], client_id) == 0){
                for(int j = 0; j < MAX_SESSIONS; j++){
                    if(strcmp(clients[i]->sessions[j]->id, session_id) == 0){
                        cur_session = clients[i]->sessions[j];
                        sess_found = true;
                        break;
                    }
                }
            }
            if(sess_found) break;
        }
        for (i = 0; i < NUM_USERS; i++){
            for(int j = 0; j < MAX_SESSIONS; j++){
                if (clients[i] != NULL && clients[i]->is_active  && (clients[i]->sessions[j] != NULL && strcmp(clients[i]->sessions[j]->id, cur_session->id)==0)) {
                    strcpy(from, cur_session->id);
                    sprintf(ack_msg, "%d:%d:%s:%s", MESSAGE, strlen(msg_data), from, msg_data);
                    send(clients[i]->fd, ack_msg, strlen(ack_msg), 0);
                    break;
                    //send the message to this client
                } 
            }
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
            if(clients[i] != NULL && clients[i]->is_active){ 
                int s_fd = clients[i]->fd;
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
                int s_fd = -1;
                if(clients[i] != NULL){
                    s_fd = clients[i]->fd;
                }

                if(FD_ISSET(s_fd, &sock_set)){
                    //printf("%d\n",s_fd);
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

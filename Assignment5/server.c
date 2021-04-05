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
#include <time.h>

#define MAX_ID 100
#define MAX_HOST 10
#define BACKLOG 10 
#define NUM_USERS 5
#define MAX_SESSIONS 3
#define SOCKET_TIMEOUT 1000
#define SELECT_TIMEOUT 1000

#include "message.h"

char MYHOST[MAX_HOST];

//session struct
struct Session {
    char id[MAX_ID];
    int count;
}; 

//client struct
struct Client {
    char user_id[MAX_ID];
    int fd;
    bool is_active;
    bool has_sessions;
    struct Session *sessions[MAX_SESSIONS];
    time_t to_start;
};

struct Client *clients[NUM_USERS] = {NULL, NULL, NULL, NULL, NULL};


FILE * fPtr;
//login info
const char* users[] = {"u1", "u2", "Navid", "YourMom", "Hamid"};
const char* pwds[] = {"p1", "p2", "yellow", "green", "blue"};

//handles client request based on parsed message received
void command_handler(struct Message* msg, int client_fd){
    int type = msg->type;
    char ack_msg[MAX_OVER_NETWORK];
    memset(ack_msg,0,sizeof(ack_msg));
    char* source = "Server";
    char from[100];
    bool authorized = false;
    int i;
    if(type == LOGIN){ //login request
        for(i = 0; i < NUM_USERS; i++){
            if((strcmp(users[i], (char*)msg->source)==0) && (strcmp(pwds[i], (char*)msg->data)==0)){ //check if username and password are correct 
                if(clients[i] == NULL || !clients[i]->is_active){ //check if the client has not logged in yet
                    if(clients[i] == NULL){ //if the client has never logged in
                        struct Client *to_add;
                        to_add = malloc(sizeof(struct Client));
                        to_add->fd = client_fd;
                        strcpy(to_add->user_id, users[i]);
                        to_add->is_active = true;
                        to_add->has_sessions = false;
                        for(int j = 0; j < MAX_SESSIONS; j++){
                            to_add->sessions[j] = NULL;
                        }
                        clients[i] = to_add;
                    }else{ //if the client has logged in previously but went inactive
                        clients[i]->fd = client_fd;
                        clients[i]->is_active = true;
                    }
                    clients[i]->to_start = time(NULL); //setting the clients start time for timeout purposes
                    char* data = " ";
                    sprintf(ack_msg, "%d:%d:%s:%s", LO_ACK, strlen(data), source, data);
                    send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgement
                    authorized = true;
                    break;
                }else if(clients[i]->is_active){ //if the client is active this user has already logged in
                    char* data = "Already logged in";
                    sprintf(ack_msg, "%d:%d:%s:%s", LO_NACK, strlen(data), msg->source, data);
                    send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgement
                    authorized = true;
                    break;
                }
            }
            if(authorized) break;
        }
        if(!authorized && (clients[i] == NULL || !clients[i]->is_active)){ //incorrect login info given
            char* data = "Incorrect login info";
            sprintf(ack_msg, "%d:%d:%s:%s", LO_NACK, strlen(data), source, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);//send appropriate acknowledgement
            close(client_fd);
        }
    }else if(type == EXIT){ //logout or quit request
        char *client_id = msg->source;
        for (int i = 0; i < NUM_USERS; i++){
            if (strcmp(users[i], client_id) == 0) { //set the client to inactive and remove its sessions 
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
        close(client_fd); //close the client socket
    }else if(type == JOIN){ //join session request
        char *session_id = (char *)msg->data;
        char *client_id = (char *)msg->source;
        bool sess_found = false;

        int i;
        //check if the session you are looking for exists
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
        bool added = false;
        if(sess_found){ //if the session exists add it to the clients sessions list
            struct Session *to_add;
            to_add = malloc(sizeof(struct Session));
            strcpy(to_add->id, session_id);
            for(i = 0; i < NUM_USERS; i++){
                if(strcmp(users[i], client_id) == 0){
                    for(int j = 0; j < MAX_SESSIONS; j++){ //looking for an empty session to add it to
                        if(clients[i]->sessions[j] == NULL){
                            clients[i]->sessions[j] = to_add;
                            clients[i]->has_sessions = true;
                            added = true;
                            break;
                        }
                    }
                    break;
                }
            }
            if(added){ //if it has been added 
                char data[MAX_OVER_NETWORK];
                memset(data, 0, sizeof(data));
                sprintf(data, "Session %s joined", session_id);
                sprintf(ack_msg, "%d:%d:%s:%s", JN_ACK, strlen(data), session_id, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgement
                printf("Session joined\n");
            }else{ //if client has reached a limit of max sessions
                char data[MAX_OVER_NETWORK];
                memset(data, 0, sizeof(data));
                sprintf(data, "Max number of sessions reached, could not join session %s", session_id);
                sprintf(ack_msg, "%d:%d:%s:%s", JN_ACK, strlen(data), session_id, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgement
                printf("Session joined\n");
            }
        }else{ //if the session does not exist
            char data[MAX_OVER_NETWORK];
            memset(data, 0, sizeof(data));
            sprintf(data, "Session %s not found", session_id);
            sprintf(ack_msg, "%d:%d:%s:%s", JN_NACK, strlen(data), session_id, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgement
            printf("Session not found\n");
        }
    }else if(type == LEAVE_SESS){ //leave session request
        char *client_id = msg->source;
        char *session_id = msg->data;

        for(i = 0; i < NUM_USERS; i++){ 
            if(strcmp(users[i], client_id) == 0){
                if(clients[i]->has_sessions){ //set the appropriate session to NULL
                    for(int j = 0; j < MAX_SESSIONS; j++){
                        if(clients[i]->sessions[j] != NULL && strcmp(clients[i]->sessions[j]->id, session_id) == 0){
                            clients[i]->sessions[j] = NULL;
                            break;
                        }
                    }
                    char* data = "Left session";
                    sprintf(ack_msg, "%d:%d:%s:%s", LS_ACK, strlen(data), msg->source, data);
                    send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgment
                    break;
                }
            }
        }
    }else if(type == NEW_SESS){ //create session results 
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
                        if(clients[i]->sessions[j] == NULL){ //check if theres space to add a session
                            if(empty == -1){
                                empty = j;
                            }
                        }else if(strcmp(clients[i]->sessions[j]->id, session_id) == 0){ //check if the session already exists
                            sess_found = true;
                        }
                    }
                    if(!sess_found){ //if session does not exist
                        if(empty == -1){ //if there is no space
                            char data[MAX_OVER_NETWORK];
                            memset(data, 0, sizeof(data));
                            sprintf(data, "Max number of sessions reached", session_id);
                            sprintf(ack_msg, "%d:%d:%s:%s", NS_NACK, strlen(data), session_id, data);
                            send(client_fd, ack_msg, strlen(ack_msg), 0);
                            break;
                        }//if there is space for the session to be added
                        clients[i]->has_sessions = true;
                        clients[i]->sessions[empty] = to_add;
                        char data[MAX_OVER_NETWORK];
                        memset(data, 0, sizeof(data));
                        sprintf(data, "Session %s created", session_id);
                        sprintf(ack_msg, "%d:%d:%s:%s", NS_ACK, strlen(data), session_id, data);
                        send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgment
                        break;
                    }else if(sess_found){ //if session already exists
                        char data[MAX_OVER_NETWORK];
                        memset(data, 0, sizeof(data));
                        sprintf(data, "Session %s already exists", session_id);
                        sprintf(ack_msg, "%d:%d:%s:%s", NS_NACK, strlen(data), session_id, data);
                        send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgment
                        break;
                    }
                }
                else if(!clients[i]->has_sessions){ //if client has no sessions just add as the first member of the list
                    clients[i]->has_sessions = true;
                    clients[i]->sessions[0] = to_add;
                    char data[MAX_OVER_NETWORK];
                    memset(data, 0, sizeof(data));
                    sprintf(data, "Session %s created", session_id);
                    sprintf(ack_msg, "%d:%d:%s:%s", NS_ACK, strlen(data), session_id, data);
                    send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgment
                    break;
                }
            }
        }
    }else if(type == QUERY){ //list request
        char data[MAX_OVER_NETWORK];
        memset(data, 0, sizeof(data));
        int cnt_sessions = 0;
        for (i = 0; i < NUM_USERS; i++){
            if (clients[i] != NULL && clients[i]->is_active){ //for an active client
                cnt_sessions = 0;
                sprintf(data + strlen(data), "%s:", users[i]); //add the user id to the data 
                for(int j = 0; j < MAX_SESSIONS; j++){
                    if(clients[i]->sessions[j] != NULL){ //if the client has a session 
                        cnt_sessions++;
                        sprintf(data + strlen(data), "%s,", clients[i]->sessions[j]->id); //add the session to the data
                    }
                }
                if(cnt_sessions == 0){
                    sprintf(data + strlen(data), "No Sessions\n"); //if the client has no sessions
                }else{
                    sprintf(data + strlen(data), "\n");
                }
            }
        }
        sprintf(ack_msg, "%d:%d:%s:%s", QU_ACK, strlen(data), msg->source, data); 
        send(client_fd, ack_msg, strlen(ack_msg), 0); //send appropriate acknowledgement
        //send a message of all online users and available sessions
    }else if(type == MESSAGE){
        //loop through sockets in specified conference session sending the message
        char *client_id = msg->source;
        char inputPre[strlen(msg->data)];
        strcpy(inputPre, msg->data);
        char *session_id = strtok(inputPre, " ");
        char *msg_data = inputPre + strlen(inputPre) + 1; //to find theRest
        int fd;

        struct Session *cur_session = NULL;
        bool sess_found = false;
        for (i = 0; i < NUM_USERS; i++){ //loop to find if the session exists
            if (clients[i] != NULL && clients[i]->is_active && strcmp(users[i], client_id) == 0){
                fd = clients[i]->fd;
                for(int j = 0; j < MAX_SESSIONS; j++){
                    if(clients[i]->sessions[j] != NULL && strcmp(clients[i]->sessions[j]->id, session_id) == 0){
                        cur_session = clients[i]->sessions[j];
                        sess_found = true;
                        break;
                    }
                }
            }
            if(sess_found) break;
        }
        if(sess_found){ //if the client has the session send it to the clients also in the session
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
        }else{ //if the client was not in session
            for (i = 0; i < NUM_USERS; i++){
                if (clients[i] != NULL && clients[i]->is_active && strcmp(users[i], client_id) == 0){
                    fd = clients[i]->fd;
                }
            }
            char data[MAX_OVER_NETWORK];
            sprintf(data, "Not in session %s",session_id);
            sprintf(ack_msg, "%d:%d:%s:%s", MESSAGE_NACK, strlen(msg_data), source, msg_data);
            send(fd, ack_msg, strlen(ack_msg), 0);
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
        struct timeval timeout;
        timeout.tv_sec = SELECT_TIMEOUT;
        timeout.tv_usec = 0;

        FD_ZERO(&sock_set);
        FD_SET(serv_fd, &sock_set);


        for(int i = 0; i < NUM_USERS; i++){  //add the active client fds to the set
            if(clients[i] != NULL && clients[i]->is_active){ 
                int s_fd = clients[i]->fd;
                if(s_fd > 0)
                    FD_SET(s_fd, &sock_set);
                
                if(s_fd > max_sd)
                    max_sd = s_fd;
            }
        }
        //select server or client to handle
        int sel_return = select(max_sd+1, &sock_set, NULL, NULL, &timeout);
        if (sel_return == 0){
            //puts("YOU timed out");
        }
        else if(sel_return < 0){
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
            if(rec_num_bytes < 0){
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
                clock_t to_end;
                int time_passed;
                if(clients[i] != NULL && clients[i]->is_active){ //if the client is active update the time passed
                    s_fd = clients[i]->fd;
                    to_end = time(NULL);
                    time_passed = to_end - clients[i]->to_start;
                    printf("This socket %s this much time pass: %d\n", clients[i]->user_id,time_passed);
                }

                if(FD_ISSET(s_fd, &sock_set)){ //if current socket fd is active and has data to send
                    rec_num_bytes = recv(s_fd, message_str, MAX_OVER_NETWORK, 0);
                    clients[i]->to_start = time(NULL);
                    time_passed = 0;
                    if(rec_num_bytes < 0){
                        perror("Error receiving message:");
                        return -1;
                    }

                    message_str[rec_num_bytes] = '\0';

                    struct Message msg;
                    parse_message(message_str, &msg);

                    command_handler(&msg, s_fd);
                }

                if (time_passed >= SOCKET_TIMEOUT){ //if time passes is greater than or equal to timeout value
                    time_passed = 0;
                    struct Message fake_msg; //create fake message to logout the client and send to client that it has been timed out
                    fake_msg.type = EXIT;
                    fake_msg.size = 0;
                    strcpy(fake_msg.source, clients[i]->user_id);
                    strcpy(fake_msg.data, "");
                    char ack_msg[MAX_OVER_NETWORK];
                    sprintf(ack_msg, "%d:%d:%s:%s", fake_msg.type, fake_msg.size , "SERVER", fake_msg.data);
                    send(s_fd, ack_msg, strlen(ack_msg), 0);
                    command_handler(&fake_msg, s_fd);
                }
                
            }
            
        }
    }

    close(serv_fd);
}

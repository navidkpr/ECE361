#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define MAX_NAME 25
#define MAX_DATA 500
#define MAX_OVER_NETWORK 600

struct Message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

enum MessageType{LOGIN, LO_ACK, LO_NACK, EXIT, JOIN, JN_ACK, JN_NACK, LEAVE_SESS, NEW_SESS, NS_ACK, NS_NACK, MESSAGE, QUERY, QU_ACK, LS_ACK, LS_NACK};

void parse_message(char *str, struct Message* msg);

#endif /* _MESSAGE_H_ */

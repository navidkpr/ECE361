#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define MAX_NAME 25
#define MAX_DATA 500

struct Message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

enum MessageType{LOGIN, LO_ACK, LO_NACK, EXIT, JOIN, JN_ACK, JN_NACK, LEAVE_SESS, NEW_SESS, NS_ACK, MESSAGE, QUERY, QU_ACK};

#endif /* _MESSAGE_H_ */
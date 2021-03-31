#include "message.h"
#include <string.h>


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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main() {

    char *str = "32:2:10:foobar.txt:lo World!\n";
    
    int total_frag = 0;
    int frag_no = 0;
    int packet_size = 0;

    int pt = 0;
    while (str[pt] != ':') {
        total_frag = total_frag * 10 + str[pt] - '0';
        pt++;
    }

    pt++;
    while (str[pt] != ':') {
        frag_no = frag_no * 10 + str[pt] - '0';
        pt++;
    }

    pt++;
    while (str[pt] != ':') {
        packet_size = packet_size * 10 + str[pt] - '0';
        pt++;
    }

    pt++;
    int st_pt = pt;
    while (str[pt] != ':')
        pt++;


    char *file_name = malloc( sizeof(char) * ( pt - st_pt + 1 ) );
    memcpy (file_name, &str[st_pt], pt - st_pt);
    file_name[pt - st_pt] = '\0';
    




    pt++;
    st_pt = pt;
    
    char *content = malloc( sizeof(char) * ( packet_size + 1 ) );
    memcpy (content, &str[st_pt], packet_size);
    content[packet_size] = '\0';

    printf("%d %d %d\n", total_frag, frag_no, packet_size);
    puts(file_name);
    puts(content);

    FILE * fPtr;


    /* 
     * Open file in w (write) mode. 
     * "data/file1.txt" is complete path to create file
     */
    
    if (frag_no == 1)
        fPtr = fopen(file_name, "w");
    else
        fPtr = fopen(file_name, "a");

    fputs(content, fPtr);

    if (frag_no == total_frag)
        fclose(fPtr);

}

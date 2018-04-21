#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <time.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>

#define MAXBUF 1400
#define MAXHANDLE 100
#define MAX_HANDLES 9
#define MESSAGE 5
#define BROADCAST 4
#define REQUEST 1
#define LIST 10
#define EXIT 8
#define SHORT 2
#define BYTE 1

uint8_t *recieve_packet(int socketNum);
uint16_t get_length(uint8_t *packet);
uint8_t get_flag(uint8_t *packet);
void get_sender_handle(uint8_t *packet, char *handle);

uint8_t *recieve_packet(int socketNum) {
    int read;
    int packet_length;
    static uint8_t buf[MAXBUF];
    if ((read = recv(socketNum, buf, 2, MSG_WAITALL)) == 0) {
	close(socketNum);
        return NULL;
    }
    else if (read < 0)
	perror("Socket");
    packet_length = get_length(buf);
    if ((read = recv(socketNum, buf + 2, packet_length-2, MSG_WAITALL)) == 0) {
	close(socketNum);
        return NULL;
    }
    else if (read < 0)
	perror("Socket");
    return buf;
}

uint16_t get_length(uint8_t *packet) {
    uint16_t packet_length;
    packet_length = ntohs(*(uint16_t *)packet);
    return packet_length;
}

uint8_t get_flag(uint8_t *packet) {
    uint8_t flag = *(packet + SHORT);
    return flag;
}

void get_sender_handle(uint8_t *packet, char *handle) {
    int i;
    uint8_t handle_len = *(packet + SHORT + BYTE);
    for (i = 0; i < handle_len; i++) 
	handle[i] = packet[SHORT + BYTE*2 + i];
    handle[handle_len] = '\0';
}

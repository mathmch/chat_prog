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

#include "util.h"

uint8_t *recieve_packet(int socketNum);
uint16_t get_length(uint8_t *packet);
uint8_t get_flag(uint8_t *packet);
void get_sender_handle(uint8_t *packet, char *handle);
uint8_t get_num_dests(uint8_t *packet);
void get_dest_handles(uint8_t *packet, uint8_t num_dests, char *dest_handles[MAX_HANDLES]);
void get_message(uint8_t *packet, uint8_t flag, char *message);

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
uint8_t get_num_dests(uint8_t *packet) {
    uint8_t num_handles;
    int sender_handle_len;
    char sender_handle[MAXHANDLE];
    get_sender_handle(packet, sender_handle);
    sender_handle_len = strlen(sender_handle);
    num_handles = *(packet + SHORT + BYTE*2 + sender_handle_len);
    return num_handles;
}
/* this call is destructive to the packet, make a copy of it before using */
void get_dest_handles(uint8_t *packet, uint8_t num_dests,
		      char *dest_handles[MAX_HANDLES + 1]) {
    int sender_handle_len;
    int handles_offset;
    uint8_t dest_handle_len;
    uint8_t next_handle_len;
    int i;
    char sender_handle[MAXHANDLE];
    get_sender_handle(packet, sender_handle);
    sender_handle_len = strlen(sender_handle);
    dest_handles[0] = sender_handle;
    handles_offset = SHORT + BYTE*2 + sender_handle_len + BYTE;
    dest_handle_len = *(packet + handles_offset);
    for (i = 0; i < num_dests; i++) {
	dest_handles[i+1] = (char *)(packet + handles_offset + BYTE);
	next_handle_len = *(packet + handles_offset + BYTE + dest_handle_len);
	dest_handles[i+1][dest_handle_len] = '\0';
	handles_offset += BYTE + dest_handle_len;
	dest_handle_len = next_handle_len;
    }
}
/* type = 4 broadcast, else standard */
void get_message(uint8_t *packet, uint8_t flag, char *message) {
    uint8_t sender_handle_len;
    int handles_offset;
    uint8_t dest_handle_len;
    uint8_t num_dests;
    uint8_t packet_len = get_length(packet);
    int i;
    sender_handle_len = *(packet + SHORT + BYTE);
    if (flag == BROADCAST){
	safe_memcpy(message, packet + SHORT + BYTE*2 + sender_handle_len,
		    packet_len - (SHORT + BYTE*2 + sender_handle_len));
	message[packet_len - (SHORT + BYTE*2 + sender_handle_len)] = '\0';
	return;
    }
    num_dests = get_num_dests(packet);
    handles_offset = SHORT + BYTE*2 + sender_handle_len + BYTE;
    for (i = 0; i < num_dests; i++) {
	dest_handle_len = *(packet + handles_offset);
	handles_offset += BYTE + dest_handle_len; 	    
    }
    safe_memcpy(message, packet + handles_offset, packet_len - handles_offset);
    message[packet_len - handles_offset] = '\0';
}


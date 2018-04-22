#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include "util.h"

uint8_t packet[MAXBUF];

/* for flags 2, 3, 8, 9, 10, 13 */
uint8_t *basic_packet(uint8_t flag) {
    uint16_t length = BASIC_LENGTH;
    length = htons(length);
    safe_memcpy(packet, &length, SHORT);
    packet[2] = flag;
    return packet;
}

/* for flags 1, 7, 12 */
uint8_t *handle_packet(uint8_t flag, char *handle) {
    uint8_t handle_len = strlen(handle);
    uint16_t length = BASIC_LENGTH + BYTE + handle_len;
    length = htons(length);
    safe_memcpy(packet, &length, SHORT);
    packet[2] = flag;
    packet[3] = handle_len;
    safe_memcpy(packet + BASIC_LENGTH + BYTE, handle, handle_len);
    return packet;
}
/* flag 4 only */
uint8_t *broadcast_packet(uint8_t flag, char *handle, char *message) {
    uint8_t handle_len = strlen(handle);
    uint8_t message_len = strlen(message);
    uint16_t length = BASIC_LENGTH + BYTE + handle_len + message_len;
    length = htons(length);
    safe_memcpy(packet, &length, SHORT);
    packet[2] = flag;
    packet[3] = handle_len;
    safe_memcpy(packet + BASIC_LENGTH + BYTE, handle, handle_len);
    safe_memcpy(packet + BASIC_LENGTH + BYTE + handle_len, message, message_len);
    return packet;
}

/* flag 5 only, handles 1 or multiple destinations */
uint8_t *message_packet(uint8_t flag, char *handles[], uint8_t num_dests,
			char *message) {
    int i;
    uint8_t handle_len = strlen(handles[0]);
    uint8_t message_len = strlen(message);
    uint16_t length = BASIC_LENGTH + BYTE;
    packet[2] = flag;
    packet[3] = handle_len;
    safe_memcpy(packet + BASIC_LENGTH + BYTE, handles[0], handle_len);
    length += handle_len;
    packet[length] = num_dests;
    length += BYTE;
    /* write a handle len and handle combo for each destination */
    for (i = 0; i < num_dests; i++) {
	handle_len = strlen(handles[i+1]);
	packet[length] = handle_len;
	length += BYTE;
        safe_memcpy(packet + length, handles[i+1], handle_len);
	length += handle_len;
    }
    safe_memcpy(packet + length, message, message_len);
    length += message_len;
    /* write the total packet length */
    length = htons(length);
    safe_memcpy(packet, &length, SHORT);
    return packet;
}

/* flag 11 only */
uint8_t *count_packet(uint8_t flag, uint32_t known_handles) {
    uint16_t length = BASIC_LENGTH + BYTE*4;
    length = htons(length);
    safe_memcpy(packet, &length, SHORT);
    packet[2] = flag;
    known_handles = htonl(known_handles);
    safe_memcpy(packet + BASIC_LENGTH, &known_handles, INT);
    return packet;
}

/* function to be called by the server or client when they need a packet */
uint8_t *write_packet(uint8_t flag, uint8_t num_dests, char *handles[MAX_HANDLES +1],
		      char *message, uint32_t known_handles) {
    if (flag == REQUEST) {
	return handle_packet(flag, handles[0]);
    }
    else if (flag == ACCEPT) {
	return basic_packet(flag);
    }
    else if (flag == REJECT) {
	return basic_packet(flag);
    }
    else if (flag == BROADCAST) {
	return broadcast_packet(flag, handles[0], message);
    }
    else if (flag == MESSAGE) {
	return message_packet(flag, handles, num_dests, message);
    }
    else if (flag == 6) {
        ;/* not in use */
    }
    else if (flag == UNKNOWN_HANDLE) {
	return handle_packet(flag, handles[1]);
    }
    else if (flag == EXIT) {
	return basic_packet(flag);
    }
    else if (flag == EXIT_OK) {
	return basic_packet(flag);
    }
    else if (flag == LIST) {
	return basic_packet(flag);
    }
    else if (flag == LIST_NUM) {
	return count_packet(flag, known_handles);
    }
    else if (flag == LIST_HANDLE) {
	return handle_packet(flag, handles[0]);
    }
    else if (flag == LIST_END) {
	return basic_packet(flag);
    }
    else 
	return NULL;
    return NULL;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include "util.h"

uint8_t packet[MAXBUF];

uint8_t *basic_packet(uint8_t flag) {
    uint16_t length = BASIC_LENGTH;
    length = htons(length);
    safe_memcpy(packet, &length, SHORT);
    packet[2] = flag;
    return packet;
}

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

uint8_t *count_packet(uint8_t flag, uint32_t known_handles) {
    uint16_t length = BASIC_LENGTH + BYTE*4;
    length = htons(length);
    safe_memcpy(packet, &length, SHORT);
    packet[2] = flag;
    known_handles = htonl(known_handles);
    safe_memcpy(packet + BASIC_LENGTH, &known_handles, INT);
    return packet;
}

uint8_t *write_packet(uint8_t flag, uint8_t num_dests, char *handles[MAX_HANDLES +1],
		      char *message, uint32_t known_handles) {
    if (flag == 1) {
	return handle_packet(flag, handles[0]);
    }
    else if (flag == 2) {
	return basic_packet(flag);
    }
    else if (flag == 3) {
	return basic_packet(flag);
    }
    else if (flag == 4) {
	return broadcast_packet(flag, handles[0], message);
    }
    else if (flag == 5) {
	return message_packet(flag, handles, num_dests, message);
    }
    else if (flag == 6) {
        ;/* not in use */
    }
    else if (flag == 7) {
	return handle_packet(flag, handles[1]);
    }
    else if (flag == 8) {
	return basic_packet(flag);
    }
    else if (flag == 9) {
	return basic_packet(flag);
    }
    else if (flag == 10) {
	return basic_packet(flag);
    }
    else if (flag == 11) {
	return count_packet(flag, known_handles);
    }
    else if (flag == 12) {
	return handle_packet(flag, handles[0]);
    }
    else if (flag == 13) {
	return basic_packet(flag);
    }
    else 
	return NULL;
    return NULL;
}

/* debug code 
int main(int argc, char *argv[]) {
    uint8_t *packet1;
    uint8_t *packet2;
    uint8_t *packet3;
    uint8_t *packet4;
    uint8_t *packet5;
    uint8_t *packet6;
    char message[100];
    char *stuff[] = {"matt", "handle1", "handle2", "handle3", "handle4"};
    memcpy(message, "hello world stuff", 18);	
    packet1 = write_packet(1, 0, stuff, NULL, 0);
    packet2 = write_packet(2, 0, stuff, NULL, 0);
    packet3 = write_packet(4, 0, stuff, message, 0);
    packet4 = write_packet(5, 1, stuff, message, 0);
    packet5 = write_packet(6, 4, stuff, message, 0);
    packet6 = write_packet(11, 0, stuff, NULL, 5); 
    return 0;
}
*/

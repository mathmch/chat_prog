#ifndef RECIEVE_H
#define RECIEVE_H

uint8_t *recieve_packet(int socketNum);

uint16_t get_length(uint8_t *packet);

uint8_t get_flag(uint8_t *packet);

void get_sender_handle(uint8_t *packet, char *handle);

#endif

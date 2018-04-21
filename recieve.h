#ifndef RECIEVE_H
#define RECIEVE_H

#define MAXHANDLE 100
uint8_t *recieve_packet(int socketNum);

uint16_t get_length(uint8_t *packet);

uint8_t get_flag(uint8_t *packet);

void get_sender_handle(uint8_t *packet, char *handle);

uint8_t get_num_dests(uint8_t *packet);

void get_dest_handles(uint8_t *packet, uint8_t num_dests, char dest_handles[][MAXHANDLE]);

char *get_message(uint8_t *packet, uint8_t flag, char *message);
    
#endif

#ifndef PACKET_WRITER_H
#define PACKET_WRITER_H

#define MAX_HANDLES 9

uint8_t *write_packet(uint8_t flag, uint8_t num_dests, char *handles[MAX_HANDLES],
		      char *message, uint32_t known_handles);

#endif

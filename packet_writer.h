#ifndef PACKET_WRITER_H
#define PACKET_WRITER_H

uint8_t *write_packet(uint8_t flag, uint8_t num_dests, char *handles[],
		      char *message, uint32_t known_handles);

#endif

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

uint8_t *recieve_packet(int socketNum);

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
    packet_length = ntohs(*(uint16_t *)buf);
    if ((read = recv(socketNum, buf + 2, packet_length-2, MSG_WAITALL)) == 0) {
	close(socketNum);
        return NULL;
    }
    else if (read < 0)
	perror("Socket");
    return buf;
}

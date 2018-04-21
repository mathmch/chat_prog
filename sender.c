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
#include <inttypes.h>
#include <time.h>
#include <sys/select.h>

void safeSend(int socketNum, uint8_t *packet) {
    int sendLen = 0;        
    int sent = 0;
    sendLen = *((short *)packet);
    sendLen = ntohs(sendLen);
    sent =  send(socketNum, packet, sendLen, 0);
    if (sent < 0) {
	perror("send call");
	exit(EXIT_FAILURE);
    }
}

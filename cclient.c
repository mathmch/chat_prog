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
#include <unistd.h>

#include "networks.h"
#include "packet_writer.h"

#define MAXBUF 1400
#define xstr(a) str(a)
#define str(a) #a

void sendToServer(int socketNum);
void checkArgs(int argc, char * argv[]);
int initialize_connection(int argc, char * argv[]);
int wait_for_input(int socketNum);
void build_fdset(fd_set *fd_set, int socketNum);
int read_stdin();
int read_packet();

int main(int argc, char * argv[]) {
    int socketNum;
    int exit = 0;
    socketNum = initialize_connection(argc, argv);
    while (exit != 1){
        write(STDOUT_FILENO, "$: ", 3);
	exit = wait_for_input(socketNum);
    }  
    close(socketNum);	
    return 0;
}

void sendToServer(int socketNum) {
    char sendBuf[MAXBUF];   //data buffer
    int sendLen = 0;        //amount of data to send
    int sent = 0;            //actual amount of data sent
			
    printf("Enter the data to send: ");
    fgets(sendBuf, MAXBUF, stdin);
		
    sent =  send(socketNum, sendBuf, sendLen, 0);
    if (sent < 0) {
	perror("send call");
	exit(EXIT_FAILURE);
    }
}

void checkArgs(int argc, char * argv[]) {
    /* check command line arguments  */
    if (argc != 4) {
	printf("usage: %s handle host-name port-number \n", argv[0]);
	exit(EXIT_FAILURE);
    }
}

int initialize_connection(int argc, char * argv[]) {
    int socketNum;
    checkArgs(argc, argv);
    socketNum = tcpClientSetup(argv[2], argv[3], 0);
    /* send handle and verify acceptance */
    return socketNum;
}

int wait_for_input(int socketNum) {
    int num_ready;
    fd_set fd_set;
    build_fdset(&fd_set, socketNum);
    if ((num_ready = select(socketNum+1, &fd_set, NULL, NULL, NULL)) < 0) {
	perror("select");
	exit(EXIT_FAILURE);
    }
    if (FD_ISSET(socketNum, &fd_set)) {
	return read_packet();
    }
    else if (FD_ISSET(STDIN_FILENO, &fd_set)) {
        return read_stdin();
    }
    return 1;	
}

void build_fdset(fd_set *fd_set, int socketNum) {
    FD_ZERO(fd_set);
    FD_SET(socketNum, fd_set);
    FD_SET(STDIN_FILENO, fd_set);
}

int read_packet() {
    return 0;
}

int read_stdin() {
    char buffer[MAXBUF+1];
    char *delim = " ";
    char *token;
    int num_read;
    if ((num_read = read(STDIN_FILENO, buffer, MAXBUF)) <= 0)
	return 0;
    buffer[num_read] = '\0';
    token = strtok(buffer, delim);
    printf("%s\n", token);
    return 0;
}

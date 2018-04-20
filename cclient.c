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

#include "networks.h"
#include "packet_writer.h"
#include "recieve.h"

#define MAXBUF 1400
#define MAXHANDLE 100
#define LOCAL 1
#define MAX_HANDLES 9
#define MESSAGE 5
#define BROADCAST 4
#define REQUEST 1
#define LIST 10
#define EXIT 8


void sendToServer(int socketNum, uint8_t *packet);
void checkArgs(int argc, char * argv[]);
int initialize_connection(int argc, char * argv[]);
int wait_for_input(int socketNum);
void build_fdset(fd_set *fd_set, int socketNum);
int read_stdin();
int read_packet(int socketNum);
int message_command(char *command);
int broadcast_command(char *command);
int list_command(char *command);
int exit_command(char *command);
int verify_handle(char *handle);

char *user_name = NULL;

int main(int argc, char * argv[]) {
    int socketNum;
    int exit = 0;
    if (argc == 2 && strtol(argv[1], NULL, 10) == 1)
	socketNum = 6;
    else
	socketNum = initialize_connection(argc, argv);
    while (exit != 1){
        write(STDOUT_FILENO, "$: ", 3);
	exit = wait_for_input(socketNum);
    }  
    close(socketNum);	
    return 0;
}

void sendToServer(int socketNum, uint8_t *packet) {
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

void checkArgs(int argc, char * argv[]) {
    /* check command line arguments  */
    if (argc != 4) {
	printf("usage: %s handle host-name port-number \n", argv[0]);
	exit(EXIT_FAILURE);
    }
}

int initialize_connection(int argc, char * argv[]) {
    int socketNum;
    uint8_t *packet;
    char *handles[1];
    checkArgs(argc, argv);
    socketNum = tcpClientSetup(argv[2], argv[3], 0);
    handles[0] = argv[1];
    packet = write_packet(REQUEST, 0, handles, NULL, 0);
    sendToServer(socketNum, packet);
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
	return read_packet(socketNum);
    }
    else if (FD_ISSET(STDIN_FILENO, &fd_set)) {
        return read_stdin();
    }
    return 1;	
}

void build_fdset(fd_set *fd_set, int socketNum) {
    FD_ZERO(fd_set);
    FD_SET(STDIN_FILENO, fd_set);
    if (socketNum == 6)
	return;
    FD_SET(socketNum, fd_set);
    
}

int read_packet(int socketNum) {
    int read;
    uint8_t *packet;
    packet = recieve_packet(socketNum);
    return 0;
}

int read_stdin() {
    char buffer[MAXBUF+1];
    char *delim = " ";
    char *token;
    int len;
    int i;
    int num_read;
    if ((num_read = read(STDIN_FILENO, buffer, MAXBUF)) <= 0)
	return 0;
    buffer[num_read] = '\0';
    token = strtok(buffer, delim);
    len = strlen(token);
    for (i = 0; i < len; i++) {
	token[i] = tolower(token[i]);
    }
    if (strcmp(token, "%m") == 0)
        return message_command(buffer + 3);
    else if (strcmp(token, "%b") == 0)
	return broadcast_command(buffer + 3);
    else if (strcmp(token, "%l") == 0)
	return list_command(buffer + 3);
    else if (strcmp(token, "%e") == 0)
	return exit_command(buffer + 3);
    else
	printf("Invalid Command\n");
    return 0;
}

int message_command(char *command) {
    char *token;
    char *handles[MAX_HANDLES];
    int num_handles = 1;
    uint8_t *packet;
    int i;
    char *delim = " ";
    token = strtok(command, delim);
    if (isdigit(*token) && strlen(token) == 1) {
	num_handles = strtol(token, NULL, 10);
	token = strtok(NULL, delim);
    }
    for (i = 0; i < num_handles; i++) {
	handles[i+1] = token;
        if (verify_handle(token) == -2) {
	    printf("Invalid handle, handle starts with a number\n");
	    return -1;
	}
	else if (verify_handle(token) == -1) {
	    printf("Invalid handle, handle longer than 100 characters: %s\n", token);
	    return -1;
	}
	if (i != num_handles-1)
	    token = strtok(NULL, delim);
    }
    packet = write_packet(MESSAGE, num_handles, handles, token + strlen(token) + 1, 0);
    write(STDOUT_FILENO, packet, 35);
    return 0;
}

int broadcast_command(char *command) {
    return 0;
}

int list_command(char *command) {
    return 0;
}

int exit_command(char *command) {
    return 0;
}

int verify_handle(char *handle) {
    int len;
    if (isdigit(*handle))
	return -2;
    len = strlen(handle);
    if (len >= MAXHANDLE)
	return -1;
    return 0;
}

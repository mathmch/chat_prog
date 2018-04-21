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
#include "sender.h"

#define MAXBUF 1400
#define MAXHANDLE 100
#define MAX_HANDLES 9
#define REQUEST 1
#define ACCEPT 2
#define REJECT 3
#define BROADCAST 4
#define MESSAGE 5
#define UNKNOWN_HANDLE 7
#define EXIT 8
#define EXIT_OK 9
#define LIST 10
#define LIST_NUM 11
#define LIST_HANDLE 12
#define LIST_END 13
#define INIT 0
#define STANDARD 1

void checkArgs(int argc, char * argv[]);
int initialize_connection(int argc, char * argv[]);
int wait_for_input(int socketNum, int init);
void build_fdset(fd_set *fd_set, int socketNum, int init);
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
    socketNum = initialize_connection(argc, argv);
    while (exit != -1){
        write(STDOUT_FILENO, "$: ", 3);
	exit = wait_for_input(socketNum, STANDARD);
    }  
    close(socketNum);	
    return 0;
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
    user_name = argv[1];
    handles[0] = user_name;
    packet = write_packet(REQUEST, 0, handles, NULL, 0);
    safeSend(socketNum, packet);
    if (wait_for_input(socketNum, INIT) == -1)
	exit(EXIT_FAILURE);
    return socketNum;
}

/* waits for stdin or socket to recieve input. Return of -1 will exit program */
int wait_for_input(int socketNum, int init) {
    int num_ready;
    fd_set fd_set;
    build_fdset(&fd_set, socketNum, init);
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

void build_fdset(fd_set *fd_set, int socketNum, int init) {
    FD_ZERO(fd_set);
    if (init)
	FD_SET(STDIN_FILENO, fd_set);
    FD_SET(socketNum, fd_set);
    
}

int read_packet(int socketNum) {
    uint8_t flag;
    uint8_t *packet;
    packet = recieve_packet(socketNum);
    flag = get_flag(packet);
    if (flag == ACCEPT)
	return 0;
    else if (flag == REJECT) {
	printf("Handle already in use: %s\n", user_name);
	fflush(stdout);
	return -1;
    }
    else if (flag == BROADCAST) 
	;
    else if (flag == MESSAGE)
	;
    else if (flag == UNKNOWN_HANDLE)
	;
    else if (flag == EXIT_OK)
	;
    else if (flag == LIST_NUM)
	;
    else if (flag == LIST_HANDLE)
	;
    else if (flag == LIST_END)
	;
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

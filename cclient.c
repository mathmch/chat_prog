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
#include "util.h"


/* TODO: edge case testing */
void checkArgs(int argc, char * argv[]);
int initialize_connection(int argc, char * argv[]);
int wait_for_input(int socketNum, int init);
void build_fdset(fd_set *fd_set, int socketNum, int init);
int read_stdin(int socketNum);
int read_packet(int socketNum);
int message_command(char *command, int socketNum);
int broadcast_command(char *command, int socketNum);
int list_command(int socketNum);
int exit_command(int socketNum);
void send_message(int socketNum, uint8_t flag, uint8_t num_dests, char *handles[], char *message);
int verify_handle(char *handle);

char *user_name = NULL;

int main(int argc, char * argv[]) {
    int socketNum;
    int status = 0;
    socketNum = initialize_connection(argc, argv);
    while (status != -1){
	if (status == 0)
	    write(STDOUT_FILENO, "$: ", 3);
        status = wait_for_input(socketNum, STANDARD);
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
    if (isdigit(*argv[1])) {
	printf("Invalid handle, handle starts with a number\n");
	exit(EXIT_FAILURE);
    }
    if (strlen(argv[1]) > MAXHANDLE){
	printf("Invalid handle, handle longer than 100 characters: %s\n", argv[1]);
	exit(EXIT_FAILURE);
    }
}

/* establish a connection to the server, secure handle */
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
        return read_stdin(socketNum);
    }
    return 1;	
}

/* creates the set of open fds for select */
void build_fdset(fd_set *fd_set, int socketNum, int init) {
    FD_ZERO(fd_set);
    if (init)
	FD_SET(STDIN_FILENO, fd_set);
    FD_SET(socketNum, fd_set);
    
}

/* reads from the socket to the server */
int read_packet(int socketNum) {
    uint8_t flag;
    uint8_t *packet;
    char message[MAXMESSAGE];
    char sender[MAXHANDLE];
    packet = recieve_packet(socketNum);
    if (packet == NULL) {
	printf("\nServer Terminated\n");
	exit(EXIT_SUCCESS);
    }
    flag = get_flag(packet);
    if (flag == ACCEPT)
	return 0;
    else if (flag == REJECT) {
	printf("Handle already in use: %s\n", user_name);
	fflush(stdout);
	return -1;
    }
    else if (flag == BROADCAST) {
	get_message(packet, BROADCAST, message);
	get_sender_handle(packet, sender);
	printf("\n%s: %s", sender, message);
	fflush(stdout);
	return 0;
    }
    else if (flag == MESSAGE) {
	get_message(packet, MESSAGE, message);
	get_sender_handle(packet, sender);
	printf("\n%s: %s", sender, message);
	fflush(stdout);
	return 0;
    }
    else if (flag == UNKNOWN_HANDLE) {
        get_sender_handle(packet, sender);
	printf("\nClient with handle %s does not exist.\n", sender);
	fflush(stdout);
        return 0;
    }
    else if (flag == EXIT_OK) {
	close(socketNum);
	exit(EXIT_SUCCESS);
    }
    else if (flag == LIST_NUM) {
	printf("Number of clients: %d\n", ntohl(*((uint32_t *)(packet + 3))));
	fflush(stdout);
	return 1;
    }
    else if (flag == LIST_HANDLE) {
        get_sender_handle(packet, sender);
	printf("  %s\n", sender);
	fflush(stdout);
	return 1;
    }
    else if (flag == LIST_END)
	return 0;
    return 0;
}

/* read commands from stdin */
int read_stdin(int socketNum) {
    char buffer[MAXBUF+1];
    char *delim = " ";
    char *token;
    int len;
    int i;
    int num_read;
    if ((num_read = read(STDIN_FILENO, buffer, MAXBUF)) <= 0)
	return 0;
    if (num_read == MAXBUF) {
	printf("Message exceeded 1400 characters, not sent.\n");
	while(getchar() != '\n'); /* flush stdin */ 
	return 0;
    }
    buffer[num_read] = '\0';
    token = strtok(buffer, delim);
    len = strlen(token);
    for (i = 0; i < len; i++) {
	token[i] = tolower(token[i]);
    }
    if (strncmp(token, "%m", 2) == 0)
        return message_command(buffer + 3, socketNum);
    else if (strncmp(token, "%b", 2) == 0)
	return broadcast_command(buffer + 3, socketNum);
    else if (strncmp(token, "%l", 2) == 0)
	return list_command(socketNum);
    else if (strncmp(token, "%e", 2) == 0)
	return exit_command(socketNum);
    else
	printf("Invalid Command\n");
    return 0;
}

/* verify and send a message (flag = 5) packet */
int message_command(char *command, int socketNum) {
    char *token;
    char *handles[MAX_HANDLES + 1];
    int num_handles = 1;
    int i;
    char *delim = " ";
    token = strtok(command, delim);
    /* get the number of destinations, if provided */
    if (isdigit(*token) && strlen(token) == 1) {
	num_handles = strtol(token, NULL, 10);
	if (num_handles > 9) {
	    printf("Number of destinations must be less than 9\n");
	    return 0;
	}
	token = strtok(NULL, delim);
    }
    handles[0] = user_name;
    for (i = 0; i < num_handles; i++) {
	if (verify_handle(token) == -2) {
	    printf("Invalid handle, handle starts with a number\n");
	    return 0;
	}
	else if (verify_handle(token) == -1) {
	    printf("Invalid handle, handle longer than 100 characters: %s\n", token);
	    return 0;
	}
	handles[i+1] = token;
	if (handles[i+1][strlen(token)-1] == '\n')
	    handles[i+1][strlen(token)-1] = '\0';
	if (i != num_handles-1)
	    token = strtok(NULL, delim);
    }
    send_message(socketNum, MESSAGE, num_handles, handles, token + strlen(token) + 1);
    return 0;
}

/* send a broadcast (flag = 4) packet */
int broadcast_command(char *command, int socketNum) {
    char *handles[1];
    handles[0] = user_name;
    send_message(socketNum, BROADCAST, 0, handles, command);
    return 0;
}

/* send a list (flag = 10) request packet */
int list_command(int socketNum) {
    uint8_t *packet = write_packet(LIST, 0, NULL, NULL, 0);
    safeSend(socketNum, packet);
    return 1;
}

/* send a exit (flag = 8) request packet */
int exit_command(int socketNum) {
    uint8_t *packet = write_packet(EXIT, 0, NULL, NULL, 0);
    safeSend(socketNum, packet);
    return 1;
}

/*ensures message is less than MAXMESSAGE, send multiple packets if needed */
void send_message(int socketNum, uint8_t flag, uint8_t num_dests, char *handles[], char *message){
    uint8_t *packet;
    int i;
    char message_piece[MAXMESSAGE + 1];
    int message_len = strlen(message);
    int over = message_len/MAXMESSAGE;
    /* if multiple packets are needed */
    if (over > 0)
	for (i = 0; i < over + 1; i++) {
	    if (message_len > MAXMESSAGE) {
		safe_memcpy(message_piece, message, MAXMESSAGE);
		message_piece[MAXMESSAGE] = '\0';
	    }
	    else {
		safe_memcpy(message_piece, message, message_len);
		message_piece[MAXMESSAGE] = '\0';
	    }
	    message_len -= MAXMESSAGE;
	    message += MAXMESSAGE;
	    packet = write_packet(flag, num_dests, handles, message_piece, 0);
	    safeSend(socketNum, packet);
	}
    /* if only one packet is needed */
    else {
	packet = write_packet(flag, num_dests, handles, message, 0);
	safeSend(socketNum, packet);
    }
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

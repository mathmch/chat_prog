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

#include "networks.h"
#include "arraylist.h"
#include "packet_writer.h"
#include "recieve.h"
#include "sender.h"

#define MAXBUF 1400
#define HANDLEBUF 100
#define MAX_HANDLES 9
#define DEFAULT_SIZE 20
#define INTRO 1
#define ACCEPT 2
#define DECLINE 3
#define BROADCAST 4
#define MESSAGE 5
#define UNKNOWN_HANDLE 7
#define EXIT 8
#define LIST 10

struct Table_Header{
    int current_entries;
    int max_entries;
    int entry_size;
    uint8_t *table;
};

struct Entry{
    uint8_t socketNum;
    char handle[HANDLEBUF];
};

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void server_operation(int serverSocket);
void initialize_table(struct Table_Header *table_header, int numEntries);
int build_fdset(fd_set *fd_set, int serverSocket, struct Table_Header *table_header);
void read_sockets(int num_ready, int serverSocket, fd_set *fd_set, struct Table_Header *table_header);
void print_table(struct Table_Header *table_header);
void process_data(int socketNum, struct Table_Header *table_header);
void determine_packet_type(int socketNum, uint8_t *packet, struct Table_Header *table_header);
void new_client(int socketNum, uint8_t *packet, struct Table_Header *table_header);
void forward_broadcast(int socketNum, uint8_t *packet, struct Table_Header *table_header);
void forward_message(int socketNum, uint8_t *packet, struct Table_Header *table_header);
void approve_disconnect(int socketNum, uint8_t *packet, struct Table_Header *table_header);
void send_list(int socketNum, uint8_t *packet, struct Table_Header *table_header);
int search_entry(char *handle, struct Table_Header *table_header);

int main(int argc, char *argv[])
    {
	int serverSocket = 0;     
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	serverSocket = tcpServerSetup(portNumber);
	server_operation(serverSocket);
	return 0;
}

int checkArgs(int argc, char *argv[]) {
    // Checks args and returns port number
    int portNumber = 0;
    
    if (argc > 2) {
	fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    if (argc == 2) {
	portNumber = atoi(argv[1]);
    }
    
    return portNumber;
}

void initialize_table(struct Table_Header *table_header, int numEntries) {
    table_header->entry_size = sizeof(struct Entry);
    table_header->max_entries = numEntries;
    table_header->current_entries = 0;
    table_header->table = create_table(numEntries, table_header->entry_size);
}

/* basic server sending and recieving */
void server_operation(int serverSocket) {
    fd_set fd_set;
    int num_ready;
    int highest_socketnum;
    struct Table_Header table_header;
    initialize_table(&table_header, DEFAULT_SIZE);
    while(1){
	highest_socketnum = build_fdset(&fd_set, serverSocket, &table_header);
	if ((num_ready = select(highest_socketnum+1, &fd_set, NULL, NULL, NULL)) < 0) {
	    perror("select");
	    exit(EXIT_FAILURE);
	}
	read_sockets(num_ready, serverSocket, &fd_set, &table_header);
    }
}

/* builds the fd_set with all active sockets */
int build_fdset(fd_set *fd_set, int serverSocket, struct Table_Header *table_header) {
    int i;
    int highest_socketnum;
    struct Entry *entry;
    FD_ZERO(fd_set);
    FD_SET(serverSocket, fd_set);
    highest_socketnum = serverSocket;
    for (i = 0; i < table_header->max_entries; i++) {
	entry = (struct Entry*)table_fetch(table_header->table, table_header->entry_size, i);
	/* check if the location had a valid entry */
	if (*((uint8_t *)entry) != 0) {
	    FD_SET(entry->socketNum, fd_set);
	    if (entry->socketNum > highest_socketnum)
		highest_socketnum = entry->socketNum;
	}
    }
    return highest_socketnum;
}

/* determines which sockets are ready for reading */
void read_sockets(int num_ready, int serverSocket, fd_set *fd_set, struct Table_Header *table_header) {
    struct Entry entry;
    struct Entry *entry_ptr;
    int i;
    int j;
    if (FD_ISSET(serverSocket, fd_set)) {
	/* accept client and register their socket as in use */
        entry.socketNum = tcpAccept(serverSocket, 1);
	if (table_header->max_entries <= entry.socketNum) {
	    table_header->table = realloc_table(table_header->table, table_header->max_entries,
					        table_header->max_entries * 2, table_header->entry_size);
	    table_header->max_entries *= 2;
	}
	memcpy(entry.handle, "Temp", 5);
	table_insert(table_header->table, &entry, table_header->entry_size, entry.socketNum);
	num_ready--;
    }
    for (i = 0; i < num_ready; i++) {
	for (j = 0; j < table_header->max_entries; j++) {
	    entry_ptr = (struct Entry*)table_fetch(table_header->table, table_header->entry_size, j);
	    /* check if the location had a valid entry */
	    if (*((uint8_t *)entry_ptr) != 0 && FD_ISSET(entry_ptr->socketNum, fd_set)) {
		process_data(entry_ptr->socketNum, table_header);
	    }
	}
    }
}

void process_data(int socketNum, struct Table_Header *table_header){
    uint8_t *packet;
    /* if socket disconnects suddenly, close and remove from table */ 
    if (NULL == (packet = recieve_packet(socketNum))) {
	table_delete(table_header->table, table_header->entry_size, socketNum);
	printf("closed\n");
    }
    else {
	determine_packet_type(socketNum, packet, table_header);
    }
}

void determine_packet_type(int socketNum, uint8_t *packet, struct Table_Header *table_header) {
    uint8_t flag = get_flag(packet);
    if (flag == INTRO)
	new_client(socketNum, packet, table_header);
    else if (flag == BROADCAST)
	forward_broadcast(socketNum, packet, table_header);
    else if (flag == MESSAGE)
	forward_message(socketNum, packet, table_header);
    else if (flag == EXIT)
	approve_disconnect(socketNum, packet, table_header);
    else if (flag == LIST)
	send_list(socketNum, packet, table_header);
    else
	return; /* invalid packet */
}

void new_client(int socketNum, uint8_t *packet, struct Table_Header *table_header) {
    struct Entry entry;
    get_sender_handle(packet, entry.handle);
    entry.socketNum = socketNum;
    /* header is already in use */
    if (search_entry(entry.handle, table_header) != -1){
	safeSend(socketNum, write_packet(DECLINE, 0, NULL, NULL, 0));
	table_delete(table_header->table, table_header->entry_size, socketNum);
	close(socketNum);
    }
    /* handle is free */
    else {
	safeSend(socketNum, write_packet(ACCEPT, 0, NULL, NULL, 0));
	table_insert(table_header->table, &entry, table_header->entry_size, entry.socketNum);
    }
}

void forward_broadcast(int socketNum, uint8_t *packet, struct Table_Header *table_header) {

}

void forward_message(int socketNum, uint8_t *packet, struct Table_Header *table_header) {
    int i;
    int forwardSocket;
    char sender[MAXHANDLE];
    uint8_t num_dests = get_num_dests(packet);
    char dest_handles[MAX_HANDLES][HANDLEBUF];
    uint8_t original_packet[MAXBUF];
    /* make a copy of the packet */
    memcpy(original_packet, packet, get_length(packet));
    get_sender_handle(packet, sender);
    get_dest_handles(packet, num_dests, dest_handles);
    for(i = 0; i < num_dests; i++) {
	/* couldn't find a destination */
        if (-1 == (forwardSocket = search_entry(dest_handles[i], table_header))) {
	    safeSend(socketNum, write_packet(UNKNOWN_HANDLE, 0, &sender, NULL, 0));
	}
	else {
	    safeSend(forwardSocket, original_packet);
	}
    }
}

void approve_disconnect(int socketNum, uint8_t *packet, struct Table_Header *table_header) {

}

void send_list(int socketNum, uint8_t *packet, struct Table_Header *table_header) {

}

/* returns the socket num of matching entry or -1 if no matches */
int search_entry(char *handle, struct Table_Header *table_header) {
    int i;
    struct Entry *entry;
    for (i = 0; i < table_header->max_entries; i++) {
	entry = table_fetch(table_header->table, table_header->entry_size, i);
	/* matching entries */
	if (strcmp(handle, entry->handle) == 0)
	    return entry->socketNum;
    }
    return -1;
}
/* Mainly a debugging function */
void print_table(struct Table_Header *table_header) {
    int i;
    struct Entry *entry;
    for (i = 0; i < table_header->max_entries; i++) {
	entry = (struct Entry*)table_fetch(table_header->table, table_header->entry_size, i);
	/* check if the location had a valid entry */
	if (*((uint8_t *)entry) != 0) 
	    printf("Handle: %s\t\t\t Socket: %d\n", entry->handle, entry->socketNum);
    }
}

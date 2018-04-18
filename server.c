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

#define MAXBUF 1024
#define HANDLEBUF 100
#define DEFAULT_SIZE 20

struct Table_Header{
    int current_entries;
    int max_entries;
    int entry_size;
    uint8_t *table;
};

struct Entry{
    uint8_t socketNum;
    int index;
    char handle[HANDLEBUF];
};

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void server_operation(int serverSocket);
void initialize_table(struct Table_Header *table_header, int numEntries);
int build_fdset(fd_set *fd_set, int serverSocket, struct Table_Header *table_header);

int main(int argc, char *argv[])
{
	int serverSocket = 0;  
	int clientSocket = 0;   
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	serverSocket = tcpServerSetup(portNumber);
	server_operation(serverSocket);
	
	// wait for client to connect
	clientSocket = tcpAccept(serverSocket, 0);
	return 0;
}

void recvFromClient(int clientSocket)
{
	char buf[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recv(clientSocket, buf, MAXBUF, 0)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	printf("Message received, length: %d Data: %s\n", messageLen, buf);
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	if (argc == 2)
	{
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

void server_operation(int serverSocket) {
    fd_set fd_set;
    int num_ready;
    int highest_socketnum;
    struct Table_Header table_header;
    initialize_table(&table_header, DEFAULT_SIZE);
    highest_socketnum = build_fdset(&fd_set, serverSocket, &table_header);
    if ((num_ready = select(highest_socketnum+1, &fd_set, NULL, NULL, NULL)) < 0) {
	perror("select");
	exit(EXIT_FAILURE);
    }
    printf("got here\n");
    

}

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

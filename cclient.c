
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

#include "networks.h"

#define MAXBUF 1024
#define xstr(a) str(a)
#define str(a) #a

void sendToServer(int socketNum);
void checkArgs(int argc, char * argv[]);
int initialize_connection(int argc, char * argv[]);

int main(int argc, char * argv[])
{
    int i;
    int socketNum;
    socketNum = initialize_connection(argc, argv);
    for (i = 0; i < 3; i++) 
	sendToServer(socketNum);
    close(socketNum);	
    return 0;
}

void sendToServer(int socketNum)
{
    char sendBuf[MAXBUF];   //data buffer
    int sendLen = 0;        //amount of data to send
    int sent = 0;            //actual amount of data sent/* get the data and send it   */
			
    printf("Enter the data to send: ");
    fgets(sendBuf, MAXBUF, stdin);
	
    sendLen = strlen(sendBuf) + 1;
    printf("read: %s len: %d\n", sendBuf, sendLen);
		
    sent =  send(socketNum, sendBuf, sendLen, 0);
    if (sent < 0)
	{
	    perror("send call");
	    exit(EXIT_FAILURE);
	}

    printf("String sent: %s \n", sendBuf);
    printf("Amount of data sent is: %d\n", sent);
}

void checkArgs(int argc, char * argv[])
{
    /* check command line arguments  */
    if (argc != 4)
	{
	    printf("usage: %s handle host-name port-number \n", argv[0]);
	    exit(EXIT_FAILURE);
	}
}

int initialize_connection(int argc, char * argv[]){
    int socketNum;
    checkArgs(argc, argv);
    socketNum = tcpClientSetup(argv[2], argv[3], 0);
    /* send handle and very acceptance */
    return socketNum;
}

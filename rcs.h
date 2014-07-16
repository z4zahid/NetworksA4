#ifndef _RCS_
#define _RCS_

#include <vector>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SYN_BIT 0
#define ACK_BIT 1
#define SEQ_NUM 2
#define ACK_NUM 3
#define CHK_SUM 4
#define DATA_INDEX 5
#define HEADER_SIZE 6

#define BUFFER_SIZE 517 // 512 + HEADER_SIZE

#define ACK_SET 'a'
#define SYN_SET 's'
#define CHK_SET 'c'

//must be less than or equal to half the size of the sequence number space
#define WINDOW_SIZE 5
#define MAX_PACKET_SIZE 800
#define MAX_RETRANSMIT 5
#define ACK_TIMEOUT 300

#define SEQUENCE_NUM 0
#define PACKET_LEN 1

typedef struct datapacket {
    char data[MAX_PACKET_SIZE + 16];
    int sequenceNum;
    int checksum;
    int totalBytes;
    int packetLen;
    datapacket(): sequenceNum(-1) {}
} DataPacket;

typedef enum state {
    NEW,
    LISTEN,
    CONNECT
} State;

typedef struct conn {
    struct sockaddr_in destination;
    int socketID;
    bool ack;
	int ackNum;
    State state; 
} Connection;

int rcsSocket();
int rcsBind(int socketID, struct sockaddr_in * addr);
int rcsGetSockName(int socketID, struct sockaddr_in * addr);
int rcsListen(int socketID);
int rcsAccept(int socketID, struct sockaddr_in *addr);
int rcsConnect(int socketID, const struct sockaddr_in * addr);
int rcsRecv(int socketID, void * rcvBuffer, int maxBytes);
int rcsSend(int socketID, const void * sendBuffer, int numBytes);
int rcsClose(int socketID);

#endif


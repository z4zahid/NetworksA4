#ifndef _RCS_
#define _RCS_

#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SYN_BIT 0
#define ACK_BIT 1
#define SEQ_NUM 2
#define ACK_NUM 3
#define DATA_INDEX 4
#define HEADER_SIZE 5

#define BUFFER_SIZE 517 // 512 + HEADER_SIZE

#define ACK_SET 'a'
#define SYN_SET 's'

//must be less than or equal to half the size of the sequence number space
#define WINDOW_SIZE 5
#define MAX_PACKET_SIZE 800
#define MAX_RETRANSMIT 5
#define ACK_TIMEOUT 300

char serverSeq = 0;
char clientSeq = 0;

pthread_mutex_t lock;
int counter;

typedef struct datapacket {
    void* data;
    int sequenceNum;
    int checksum;
    int totalBytes;
    int packetLen;
    datapacket(): sequenceNum(-1) {}
} DataPacket;

typedef struct ackpacket {
    int sequenceNum;
    int packetLen;
} AckPacket;

typedef enum state {
    NEW,
    LISTEN,
    CONNECT
} State;

typedef struct conn {
    struct sockaddr_in destination;
    int socketID;
    bool ack;
    std::vector<DataPacket> dataPackets;
    State state; 
} Connection;

std::vector<Connection> connections;

#endif

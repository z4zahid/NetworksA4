#ifndef _LIB_
#define _LIB_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>

#define CLOSE_BIT 0
#define CLOSE_ACK 1
#define SYN_BIT 2
#define ACK_BIT 3
#define SEQ_NUM 4
#define ACK_NUM 5
#define CHK_SUM 6
#define DATA_INDEX 7
#define HEADER_SIZE 8

#define BUFFER_SIZE 520 // 512 + HEADER_SIZE

#define ACK_SET 'a'
#define SYN_SET 's'
#define CHK_SET 'c'
#define CLOSE_SET 'x'

//must be less than or equal to half the size of the sequence number space
#define WINDOW_SIZE 7
#define MAX_PACKET_SIZE 800
#define MAX_RETRANSMIT 5
#define ACK_TIMEOUT 300

#define SEQUENCE_NUM 0
#define PACKET_LEN 1

#define DATA_CLOSE 0
#define DATA_SEQNUM 1
#define DATA_TOTAL 5
#define DATA_CHKSUM 9
#define DATA_PKTLEN 13
#define DATA_PKTDATA 17

#define SUCCESS 0
#define RCV_ERROR -1
#define CHECKSUM_CORRUPTED -2
#define PACKET_CORRUPTED 1
#define ALL_MAX_RETRANSMIT 2
#define SEND_ZERO_BYTES -3
#define INCOMPLETE_SYNACK -4

typedef struct datapacket {
    char data[MAX_PACKET_SIZE + 17];
    char closeBit;
    int sequenceNum;
    int checksum;
    int totalBytes;
    int packetLen;
    datapacket(): sequenceNum(-1) {}
} DataPacket;

typedef struct conn {
    struct sockaddr_in destination;
    int socketID;
    bool ack;
    int ackNum;
} Connection;

void populateDataPackets(const void* sendBuffer, int numBytes, int socketID, std::vector<DataPacket>* packets) ;
int receiveDataPacket(int socketID, DataPacket *packet, struct sockaddr_in* addr);
void sendDataPacket(int socketID, DataPacket *packet);
int getTotalPackets(int numBytes);
int getChecksum(const void* packet, int size);
int IsPacketCorrupted(DataPacket packet, int expectedBytes, int);
int allRetransmitsTimedOut(int retransmits[], int size);

void addConnection(Connection connection);
Connection getConnection(int socketID);
void removeConnection(int socketID);
struct sockaddr_in getConnectionAddr(int socketID);

#endif

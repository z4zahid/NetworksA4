#include "lib.h"
#include "rcs.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <cerrno>

using namespace std;

extern int errno;
extern ssize_t ucpRecvFrom(int sockfd, void *buf, int len, struct sockaddr_in *from);
extern int ucpSendTest(int sockfd, const void *buf, int len, const struct sockaddr_in *to);

void populateDataPackets(const void* sendBuffer, int numBytes, int socketID, vector<DataPacket>* packets) {

    int i, j;
    int numPackets = getTotalPackets(numBytes);

    for (i = 0; i<numPackets;i++) {
        
        DataPacket packet;
        packet.sequenceNum = i;
        packet.totalBytes = numBytes;
        packet.packetLen = (i == numPackets - 1)? (numBytes - MAX_PACKET_SIZE*i) : MAX_PACKET_SIZE;

        int size = packet.packetLen + 17; //4 ints to be stored as chars
        memset(packet.data, 0, size);

        int index = i*MAX_PACKET_SIZE;
        char* iterate = (char*)sendBuffer;
        for (j=0; j<index; j++){
            iterate++;
        }
 
        packet.checksum = getChecksum(iterate, packet.packetLen);

        char zero = 0;	
        memcpy(&packet.data[DATA_CLOSE], &zero, sizeof(char));
        memcpy(&packet.data[DATA_SEQNUM], &packet.sequenceNum, sizeof(int));
        memcpy(&packet.data[DATA_TOTAL], &packet.totalBytes, sizeof(int));
        memcpy(&packet.data[DATA_CHKSUM], &packet.checksum, sizeof(int));
        memcpy(&packet.data[DATA_PKTLEN], &packet.packetLen, sizeof(int));
        memcpy(&packet.data[DATA_PKTDATA], iterate, packet.packetLen);
        
        packets->push_back(packet);
    }
}

int receiveDataPacket(int socketID, DataPacket *packet, struct sockaddr_in* addr) {

    int size = MAX_PACKET_SIZE + 17;
    char data[size]; 
    memset(data, 0, size);
    int bytes = ucpRecvFrom(socketID, data, size, addr);

	if (bytes < 0) {
        errno = EPERM;
	cout << "rcv_error" << endl;
		return RCV_ERROR;
    }
    
    memcpy(&packet->closeBit, &data[DATA_CLOSE], sizeof(char));
    cout << "rcv: closeBit: " << packet->closeBit << endl;
    memcpy(&packet->sequenceNum, &data[DATA_SEQNUM], sizeof(int));
    cout << "rcv: sequenceNum: " << packet->sequenceNum << endl;
    memcpy(&packet->totalBytes, &data[DATA_TOTAL], sizeof(int));
    cout << "rcv: totalBytes: " << packet->totalBytes << endl;
    memcpy(&packet->checksum, &data[DATA_CHKSUM], sizeof(int));
    cout << "rcv: checksum: " << packet->checksum << endl;
    memcpy(&packet->packetLen, &data[DATA_PKTLEN], sizeof(int));
    cout << "rcv: len: " << packet->packetLen << endl;
	if (packet->packetLen > 0 && packet->packetLen <= MAX_PACKET_SIZE)
    	memcpy(&packet->data, &data[DATA_PKTDATA], packet->packetLen);
	
	return SUCCESS;

}

void sendDataPacket(int socketID, DataPacket *packet) {

    int size = packet->packetLen + 17; //4 ints to be stored as chars
	sockaddr_in addr = getConnectionAddr(socketID);
    ucpSendTest(socketID, packet->data, size, &addr);
}

int getTotalPackets(int numBytes) {
    int numPackets = numBytes/MAX_PACKET_SIZE;
    if (numBytes % MAX_PACKET_SIZE > 0) {
        numPackets++;
    }
    return numPackets;
}
    
// let's just sum them, ucpSend corrupts each packet, very unlikely this hash will fail
int getChecksum(const void* packet, int size) {

	if (size <0 || size > MAX_PACKET_SIZE) {
        errno = EINVAL;
		return CHECKSUM_CORRUPTED;
    }

    int sum = 0, i;
    char* it = (char*)packet;
    for (i = 0; i< size; i++) {
        sum += (int)(*it);
        it++;
    }
    return sum;
}

int IsPacketCorrupted(DataPacket packet, int expectedBytes, int bytesReceived) {

    errno = ERANGE;

	//special case (everything in packet is 0)
	if (packet.sequenceNum == 0 && packet.packetLen ==0){
		cout << "corrupted " << endl;
		return PACKET_CORRUPTED;
	}
	
	if (packet.packetLen <= 0) {
		cout << "corrupted " << endl;
		return PACKET_CORRUPTED;
	}

	int expectedPackets = getTotalPackets(expectedBytes);
	if (packet.sequenceNum == (expectedPackets-1) && (expectedBytes - bytesReceived) != packet.packetLen) {
        //this will force the last packet to be received last, but that way we know its definitely correct?
	return PACKET_CORRUPTED;
    }

	if( packet.sequenceNum < (expectedPackets-1) && packet.packetLen < MAX_PACKET_SIZE) {
		cout << "corrupted " << endl;
		return PACKET_CORRUPTED;
	}

	if (packet.totalBytes == 0 && packet.sequenceNum > 0) {
		cout << "corrupted " << endl;
		return PACKET_CORRUPTED;
	}

    //checksum
    if (getChecksum(packet.data, packet.packetLen) != packet.checksum) {
		cout << "corrupted " << endl;
        return PACKET_CORRUPTED;
    }

    errno = 0;
    return SUCCESS;
}

int allRetransmitsTimedOut(int retransmits[], int size) {
    int i;
	for ( i=0; i<size; i++) { 
        if (retransmits[i] < MAX_RETRANSMIT){
			return SUCCESS; 
 	   }
	}
    errno = EPERM;
    return ALL_MAX_RETRANSMIT;
}

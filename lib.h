#ifndef _LIB_
#define _LIB_

#include "rcs.h"
#include <vector>

void populateDataPackets(const void* sendBuffer, int numBytes, int socketID, std::vector<DataPacket>* packets) ;
int receiveDataPacket(int socketID, DataPacket *packet, struct sockaddr_in* addr);
void sendDataPacket(int socketID, DataPacket *packet);
int getTotalPackets(int numBytes);
int getChecksum(const void* packet, int size);
int IsPacketCorrupted(DataPacket packet, int expectedPackets);
int allRetransmitsTimedOut(int retransmits[], int size);

#endif

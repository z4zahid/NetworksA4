#include "ucp.c"
#include "rcs.h"

#include <iostream>
#include <pthread.h>
#include <string>
#include <cstring>

using namespace std;

char serverSeq = 0;
char clientSeq = 0;

pthread_mutex_t lock;
int counter;

// vector of sockets
vector<Connection> connections;

void addConnection(Connection connection) {
    pthread_mutex_lock(&lock);
    connections.push_back(connection);
    pthread_mutex_unlock(&lock);
}

void removeConnection(int socketID) {
    pthread_mutex_lock(&lock);
    for (int i = 0 ; i < connections.size(); i++) {
        if ((connections.at(i)).socketID == socketID) {
            connections.erase(connections.begin() + i);
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

struct sockaddr_in getConnectionAddr(int socketID) {
    pthread_mutex_lock(&lock);
    for (int i = 0 ; i < connections.size(); i++) {
        if ((connections.at(i)).socketID == socketID) {
            return connections.at(i).destination;
        }
    }
    pthread_mutex_unlock(&lock);
}

//used to allocate an RCS socket. Returns a socket descriptor (positive integer) on success
int rcsSocket() {
   return ucpSocket();
} 

//binds an RCS socket (first argument) to the address structure (second argument)
int rcsBind(int socketID, const struct sockaddr_in * addr) {
    return ucpBind(socketID, addr);
}

//fills address information into the second argument with which an RCS socket (first argument) has been 
//bound via a call to rcsBind()
int rcsGetSockName(int socketID, struct sockaddr_in * addr){
    return ucpGetSockName(socketID, addr);
}

//marks an RCS socket (the argument) as listening for connection requests
int rcsListen(int socketID) {

    //success
    return 0;
}

// accepts a connection request on a socket (the first argument).
// This is a blocking call while awaiting connection requests. 
// The call is unblocked when a connection request is received. 
// The address of the peer (client) is filled into the second argument. 
// Returns descriptor to new RCS socket that can be used to rcsSend() and rcsRecv() with the peer (client).
int rcsAccept(int socketID, struct sockaddr_in *addr) {
    char receiveBuf[BUFFER_SIZE];
    Connection connection;
    
    // Blocking call until we get SYN request
    while (true) {
        ucpRecvFrom(socketID, receiveBuf, BUFFER_SIZE, addr);
        if (receiveBuf[SYN_BIT] == SYN_SET && receiveBuf[CHK_SUM] == CHK_SET) {
            connection.destination = *addr;
            connection.socketID = socketID;
	    connection.ack = false;
            break;
        }
    }
    
    // Create SYNACK and add sequence number
    char seq_num = serverSeq++;
    char ack_num = receiveBuf[SEQ_NUM] + 1;
    char sendBuf[BUFFER_SIZE];
    sendBuf[SYN_BIT] = SYN_SET;
    sendBuf[ACK_BIT] = ACK_SET;
    sendBuf[ACK_NUM] = ack_num;
    sendBuf[SEQ_NUM] = seq_num;
    sendBuf[CHK_SUM] = CHK_SET;
    ucpSetSockRecvTimeout(socketID, 1000);
    memset(receiveBuf, 0, BUFFER_SIZE);
    

    connection.ackNum = seq_num;
    addConnection(connection);

    struct sockaddr_in * ackAddr;
    while (true) {
        ucpSendTo(socketID, sendBuf, BUFFER_SIZE, addr);
        ucpRecvFrom(socketID, receiveBuf, BUFFER_SIZE, ackAddr);
        if (receiveBuf[CHK_SUM] == CHK_SET && receiveBuf[ACK_BIT] == ACK_SET && receiveBuf[ACK_NUM] == seq_num + 1) {
            connection.ack = true;
            break;
        }
    }
    
    return 0;
}

// connects a client to a server. The socket (first argument) must have been bound beforehand using rcsBind().
// The second argument identifies the server to which connection should be attempted
int rcsConnect(int socketID, const struct sockaddr_in * addr) {
    
    // Set a timeout
    ucpSetSockRecvTimeout(socketID, 3000);
    
    char buf[BUFFER_SIZE];
    char seq_num = clientSeq++;
    buf[SYN_BIT] = SYN_SET;
    buf[SEQ_NUM] = seq_num;
    buf[CHK_SUM] = CHK_SET;
    
    char receiveBuf[BUFFER_SIZE];
    struct sockaddr_in serverAddr = *addr;
     
    // Send SYN to server and wait for ACK from the server
    while (true) {
        ucpSendTo(socketID, buf, BUFFER_SIZE, addr);
        ucpRecvFrom(socketID, receiveBuf, BUFFER_SIZE, &serverAddr);
        if (receiveBuf[CHK_SUM] == CHK_SET && receiveBuf[ACK_BIT] == ACK_SET && receiveBuf[ACK_NUM] == seq_num + 1) {
            break;
        }
    }
    
    memset(buf, 0, BUFFER_SIZE);
    char ack_num = receiveBuf[SEQ_NUM] + 1;
    buf[ACK_NUM] = ack_num;
    buf[ACK_BIT] = ACK_SET;
    buf[CHK_SUM] = CHK_SET;
        
    memset(receiveBuf, 0, BUFFER_SIZE);
    
    // Send ACK to the server
    ucpSendTo(socketID, buf, BUFFER_SIZE, addr);
    
	//success
    return 0;
}

void receiveDataPacket(int socketID, DataPacket *packet, struct sockaddr_in* addr) {

    int size = MAX_PACKET_SIZE + 16;
    char data[size]; 
    ucpRecvFrom(socketID, data, size, addr);

    memcpy(packet->sequenceNum, data[0], sizeof(int));
    memcpy(packet->totalBytes, data[4], sizeof(int));
    memcpy(packet->checksum, data[8], sizeof(int));
    memcpy(packet->packetLen, data[12], sizeof(int));
    memcpy(packet->data, data[16], packet.packetLen);
}

void sendDataPacket(int socketID, DataPacket packet) {

    int size = packet.packetLen + 16; //4 ints to be stored as chars
    char data[size]; 
    memset(data, 0, size);

    memcpy(data[0], packet.sequenceNum, sizeof(int));
    memcpy(data[4], packet.totalBytes, sizeof(int));
    memcpy(data[8], packet.checksum, sizeof(int));
    memcpy(data[12], packet.packetLen, sizeof(int));
    memcpy(data[16], packet.data, packet.packetLen);

    ucpSendTo(socketID, data, size, &getConnectionAddr(socketID));
}

int getTotalPackets(int numBytes) {
    int numPackets = numBytes/MAX_PACKET_SIZE;
    if (numBytes % MAX_PACKET_SIZE > 0) {
        numPackets++;
    }
    return numPackets;
}
    
// let's just sum them, ucpSend corrupts each packet, very unlikely this hash will fail
int getChecksum(const void* packet) {
    int sum = 0;
    char* it = (char*)packet;
    while (it) {
        sum += (int)(*it);
        it++;
    }
    return sum;
}

int IsPacketCorrupted(DataPacket packet) {

    //checksum
    if (getChecksum(packet.data) != packet.checksum)
        return 1;

    //data length
    int numBytes = 0;
    char* it = (char*)packet.data;
    while(it) {
        numBytes++;
        it++;
    }

    if (numBytes != packet.packetLen)
        return 1;

    return 0;
}

// blocks awaiting data on a socket (first argument).
// Returns the actual amount of data received. “Amount” is the number of bytes. 
// Data is sent and received reliably, so any byte that is
// returned by this call should be what was sent, and in the correct order.
int rcsRecv(int socketID, void * rcvBuffer, int maxBytes) {

    int bytesReceived = -1;
    int rcvBase = 0;
    int rcvBaseHi = (rcvBase + WINDOW_SIZE - 1);
    int expectedBytes = 0;
    int expectedPackets = 0;
    vector<DataPacket> packets;

    while (bytesReceived < expectedBytes && bytesReceived < maxBytes) {
        
        DataPacket packet;
        struct sockaddr_in addr;
        receiveDataPacket(socketID, &packet, &addr)
        
        if (expectedBytes == 0) {
            expectedBytes = packet.totalBytes;
            expectedPackets = getTotalPackets(expectedBytes);
            packets.resize(expectedPackets);
        }
        
        if (!IsPacketCorrupted(packet)) {
            if (packet.sequenceNum >= rcvBase && packet.sequenceNum <= rcvBaseHi) {

                int ack[2];
                ack[SEQUENCE_NUM] = packet.sequenceNum;
                ack[PACKET_LEN] = packet.packetLen;
                ucpSendTo(socketID, &ack, sizeof(ack), &addr);

                //If the packet was not previously received, it is buffered.
                if (packets[packet.sequenceNum].sequenceNum < 0) {
                    packets[packet.sequenceNum] = packet;
                    bytesReceived += packet.packetLen;
                }

                if (packet.sequenceNum == rcvBase) {
                    // deliver this packet and any previously buffered and consecutively numbered packets
                    int i = packet.sequenceNum;
                    while(packets[i].sequenceNum < 0) {

                        DataPacket p = packets[i];
                        int index = p.sequenceNum*MAX_PACKET_SIZE;
                        char* iterate = (char*)rcvBuffer;
                        for (i=0; i<index; i++){
                            iterate++;
                        }

                        memcpy(iterate, p.data, p.packetLen);
                    
                        i++;
                    }
                    // moved receive window forward by the number of packets delivered
                    int moveForwardBy = packet.sequenceNum - i; 
                    rcvBase += moveForwardBy;
                    rcvBaseHi = ((rcvBaseHi + moveForwardBy) > expectedPackets )? expectedPackets: rcvBaseHi + moveForwardBy;
                } 
            } else if (packet.sequenceNum >= (rcvBase - WINDOW_SIZE) && packet.sequenceNum <= (rcvBase - 1)) {
                //return ACK to sender even though we've already ACKED before
                int ack[2];
                ack[SEQUENCE_NUM] = packet.sequenceNum;
                ack[PACKET_LEN] = packet.packetLen;
                ucpSendTo(socketID, &ack, sizeof(ack), &addr); 
            } else {
                //ignore
            }
        }
    }

    return bytesReceived;
} 

int allRetransmitsTimedOut(int retransmits[], int size) {
    int i;
    for ( i=0; i<size; i++) {
        if (retransmits[i] < MAX_RETRANSMIT && retransmits[i] > 0)
            return 0; 
    }
    return 1;
}

void populateDataPackets(const void* sendBuffer, int numBytes, int socketID, vector<DataPacket>* packets) {

    int i = 0;
    int numPackets = getTotalPackets(numBytes);

    for (i = 0; i<numPackets;i++) {
        
        DataPacket dataPacket;
        dataPacket.sequenceNum = i;
        dataPacket.totalBytes = numBytes;
        dataPacket.packetLen = (i == numPackets - 1)? (numBytes - MAX_PACKET_SIZE*i) : MAX_PACKET_SIZE;

        int index = i*MAX_PACKET_SIZE;
        char* iterate = (char*)sendBuffer;
        for (i=0; i<index; i++){
            iterate++;
        }

        memset(dataPacket.data, '\0', dataPacket.packetLen);
        memcpy(dataPacket.data, iterate, dataPacket.packetLen);
        
        dataPacket.checksum = getChecksum(dataPacket.data);
        packets.push_back(dataPacket);
    }
}

/* evilKind == 0 -- send only part of the bytes 
 *             1 -- corrupt some of the bytes
 *             2 -- pretend that we sent all the bytes
 */
// blocks sending data. 
// Returns the actual number of bytes sent. If rcsSend() returns with a
// non-negative return value, then we know that so many bytes were reliably received by the other end
int rcsSend(int socketID, const void * sendBuffer, int numBytes) {
    
    vector<DataPacket> dataPackets;
    int numPackets = getTotalPackets(numBytes);
    populateDataPackets(sendBuffer, numBytes, socketID, &dataPackets);

    //let's send out all the packets and then we can wait for them
    int curSequenceNum = 0, i;
    for (i = 0; i<numPackets;i++) {
        sendDataPacket(socketID, dataPackets.at(i));
        curSequenceNum++;
    }

    int rcvPackets[numPackets];
    memset(rcvPackets, 0, numPackets);
    int retransmits[numPackets];
    memset(retransmits, 0, numPackets);
    int bytesReceived = 0;
    int curWindowLo = 0;
    int curWindowHi = (numPackets < (WINDOW_SIZE - 1))? numPackets : WINDOW_SIZE-1;

    //now we receive them and move around our window accordingly
    while (bytesReceived < numBytes && allRetransmitsTimedOut(retransmits, numPackets)) {

        ucpSetSockRecvTimeout(socketID, ACK_TIMEOUT);

        struct sockaddr_in addr;
        int ack[2];
        ssize_t bytes = ucpRecvFrom(socketID, &ack, sizeof(ack), &addr);
        
        // ACK received:
        if (bytes > 0) {
            
            int seq = ack.sequenceNum;

            // if in current window, mark packet as received
            if (seq >= curWindowLo && seq <= curWindowHi) {
                if (rcvPackets[seq]== 0) {
                    bytesReceived += ack.packetLen;
                    rcvPackets[seq] = 1;
                }
            } 

            // if this packet's sequence number = send_base
            if (seq == curWindowLo) {
                // case 1: this is the ACK we expect -> move window
                int i = seq;
                while(rcvPackets[i] != 0) {
                    i++;
                }

                int moveForwardBy = (seq - i); 
                curWindowLo += moveForwardBy;
                curWindowHi = ((curWindowHi + moveForwardBy) > numPackets )? numPackets: curWindowHi + moveForwardBy;

            } else {
                // case 2: this is greater than the ACK we expect -> retransmit ones in middle
                for (i = curWindowLo; i<seq;i++) {
                    if (rcvPackets[i] == 0 && retransmits[i] < MAX_RETRANSMIT) {
                        sendDataPacket(socketID, dataPackets.at(i));
                        retransmits[i] = retransmits[i] + 1;
                    }
                }
            }
        }
        else {
            // case 3: we did not get an ACK back at all -> retransmit the one we're expecting
            int i = curWindowLo;
            if (rcvPackets[i] == 0 && retransmits[i] < MAX_RETRANSMIT) {
                sendDataPacket(socketID, dataPackets.at(i));
                retransmits[i] = retransmits[i] + 1;
            }
        }   
    }

    return bytesReceived;
} 

//closes an RCS socket descriptor
int rcsClose(int socketID) {
    removeConnection(socketID);
	return ucpClose(socketID);
} 


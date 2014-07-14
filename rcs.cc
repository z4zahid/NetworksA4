#include <iostream>
#include <vector>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

#include "ucp_given.c"


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

using namespace std;

typedef struct connection {
    sockaddr_in destination;
    int socketID;
    bool ack;
} Connection;

char serverSeq = 0;
char clientSeq = 0;

pthread_mutex_t lock;
int counter;

//send/rcv 
typedef struct datapacket {
    void* data;
    int sequenceNum;
    int checksum;
} DataPacket;

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

sockaddr_in getConnectionAddr(int socketID) {
    pthread_mutex_lock(&lock);
    for (int i = 0 ; i < connections.size(); i++) {
        if ((connections.at(i)).socketID == socketID) {
            return connections.at(i).destination;
        }
    }
    pthread_mutex_unlock(&lock);
}

/*
server invokes: rcsSocket(), rcsBind(), rcsListen(), rcsAccept(), rcsRecv(), rcsSend() and rcsClose()
client invokes rcsSocket(), rcsBind(), rcsConnect(), rcsRecv(), rcsSend() and rcsClose()

Each should return -1 on error and set errno appropriately.
*/

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
        if (receiveBuf[SYN_BIT] == SYN_SET) {
            connection.destination = *addr;
            connection.socketID = socketID;
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

    ucpSetSockRecvTimeout(socketID, 1000);
    memset(receiveBuf, 0, BUFFER_SIZE);
    
    struct sockaddr_in * ackAddr;
    while (true) {
        ucpSendTo(socketID, sendBuf, BUFFER_SIZE, addr);
        ucpRecvFrom(socketID, receiveBuf, BUFFER_SIZE, ackAddr);
        if (receiveBuf[ACK_BIT] == ACK_SET && receiveBuf[ACK_NUM] == seq_num + 1) {
            connection.ack = true;
            addConnection(connection);
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
    
    char receiveBuf[BUFFER_SIZE];
    struct sockaddr_in serverAddr = *addr;
    
    // TODO: check if 
    
    // Send SYN to server and wait for ACK from the server
    while (true) {
        ucpSendTo(socketID, buf, BUFFER_SIZE, addr);
        ucpRecvFrom(socketID, receiveBuf, BUFFER_SIZE, &serverAddr);
        if (receiveBuf[ACK_BIT] == ACK_SET && receiveBuf[ACK_NUM] == seq_num + 1) {
            break;
        }
    }
    
    memset(buf, 0, BUFFER_SIZE);
    char ack_num = receiveBuf[SEQ_NUM] + 1;
    buf[ACK_NUM] = ack_num;
    buf[ACK_BIT] = ACK_SET;
        
    memset(receiveBuf, 0, BUFFER_SIZE);
    
    // Send ACK to the server
    ucpSendTo(socketID, buf, BUFFER_SIZE, addr);
    
    //success
    return 0;
}

// blocks awaiting data on a socket (first argument).
// Returns the actual amount of data received. “Amount” is the number of bytes. 
// Data is sent and received reliably, so any byte that is
// returned by this call should be what was sent, and in the correct order.
int rcsRecv(int socketID, void * rcvBuffer, int maxBytes) {

    DataPacket packet;

    int ackPacket[1];

    //circular buffer?
    DataPacket ackBuffer[BUFFER_SIZE];
    int readIndex = 0;
    int insertIndex = 0;

    int bytesReceived = 0;
    int curSequenceNum = 0;
    int rcvBase = 0;
    int rcvBaseHi = (rcvBase + WINDOW_SIZE - 1);

    //TODO: what's our end condition? how do we know we've received the entire message?
    //sender should send total packets in the msg as well
    while (true) {
        
        struct sockaddr_in addr;
        ucpRecvFrom(socketID, packet, maxBytes, &addr);
        
        if (packet.sequenceNum >= rcvBase && packet.sequenceNum <= rcvBaseHi) {

            if (packet.checksum + getSum(packet.data) == 0) {

                ackPacket[0] = packet.sequenceNum;
                ucpSendTo(socketID, ackPacket, sizeof(ackPacket, addr);

                // ackBuffer[insertIndex] = packet;
                // insertIndex = (insertIndex + 1) % BUFFER_SIZE;

                if (curSequenceNum == rcvBase) {
                    // deliver this packet and any previously buffered and consecutively numbered packets
                    // while(readIndex != curSequenceNum) {
                    //     rcvBuffer[curSequenceNum] = ackBuffer[readIndex].packet;
                    //     readIndex = (readIndex + 1) % BUFFER_SIZE; 
                    // }
                    // moved receive window forward by the number of packets delivered
                    rcvBase++;
                    rcvBaseHi++;
                } 
                curSequenceNum++;
            }
        } else if (packet.sequenceNum >= (rcvBase - WINDOW_SIZE) && packet.sequenceNum <= (rcvBase - 1)) {
            //return ACK to sender even though we've already ACKED before
            ackPacket[0] = packet.sequenceNum;
            ucpSendTo(socketID, ackPacket, sizeof(ackPacket), addr); 
        } else {
            //ignore
        }
    }

    return bytesReceived;
} 
    

int getSum(const void* packet){
    //we want to sum all the words

}

// checksum = 1s complement of the sum of all the bits in the segment with any overflow 
// encountered during the sum being wrapped around.
int getChecksum(const void* packet) {
    return ~(getSum(packet));
}

// use memset to get the next x bytes from sendBuffer
// create data packet with the next x number of bytes
void* getNextPacket(const void* sendBuffer, int seqNum, void* packet) {
    int index = seqNum*MAX_PACKET_SIZE, i;

    char* iterate = (char*)sendBuffer;
    for (i=0; i<index; i++){
        iterate++;
    }

    //TODO: what if there isn't max_packet_size left? add null to the end??
    memcpy ((void*)iterate, packet, MAX_PACKET_SIZE);
}

/* evilKind == 0 -- send only part of the bytes 
 *             1 -- corrupt some of the bytes
 *             2 -- pretend that we sent all the bytes
 */
// blocks sending data. 
// Returns the actual number of bytes sent. If rcsSend() returns with a
// non-negative return value, then we know that so many bytes were reliably received by the other end
int rcsSend(int socketID, const void * sendBuffer, int numBytes) {
    
    //TODO: whats the maximum number of times we retransmit a packet before we ignore it? 10 times

    int numPackets = numBytes/MAX_PACKET_SIZE;
    if (numBytes % MAX_PACKET_SIZE > 0) {
        numPackets++;
    }

    //let's send out all the packets and then we can wait for them
    int curSequenceNum = 0, i;
    for (i = 0; i<numPackets;i++) {

        DataPacket dataPacket;
        dataPacket.sequenceNum = curSequenceNum;
        dataPacket.packet = getNextPacket(sendBuffer, curSequenceNum);
        dataPacket.checksum = getChecksum(dataPacket.packet);
        
        ucpSendTo(socketID, &dataPacket, sizeof(DataPacket), getConnectionAddr(socketID));
        curSequenceNum++;
    }

    int rcvPackets[numPackets];
    memset(rcvPackets, 0, numPackets);
    int packetsReceived = 0;
    int curWindowLo = 0;
    int curWindowHi = (numPackets < WINDOW_SIZE - 1)? numPackets : WINDOW_SIZE-1;
    curSequenceNum = 0;

    //now we receive them and move around our window accordingly
    while (packetsReceived < numPackets) {

        ucpSetSockRecvTimeout(socketID, 1000);

        struct sockaddr_in addr;
        int ackPacket[2];
        ssize_t bytes = ucpRecvFrom(socketID, ackPacket, 4, &addr);
        
        // if ACK received:
        if (bytes > 0) {
            int seq = ackPacket[1];
            // if in current window, mark packet as received
            if (seq >= curWindowLo && seq <= curWindowHi) {
                packetsReceived++;
                rcvPackets[seq] = 1;
            } 

            // if this packet's sequence number = send_base
            if (seq == curWindowLo) {
                // case 1: this is the ACK we expect -> move window
                curWindowLo++;
                curWindowHi++;
            } else {
                // case 2: this is greater than the ACK we expect -> retransmit ones in middle
                for (i = curWindowLo; i<seq;i++) {

                    DataPacket dataPacket;
                    dataPacket.sequenceNum = curSequenceNum;
                    dataPacket.packet = getNextPacket(sendBuffer, curSequenceNum);
                    dataPacket.checksum = getChecksum(dataPacket.packet);
                    
                    ucpSendTo(socketID, &dataPacket, sizeof(DataPacket), getConnectionAddr(socketID));
                }
            }
        }
        else {
            // case 3: we did not get an ACK back at all -> retransmit
            // how much do we want to retransmit? one by one? all of them?
        }
            
    }

    return packetsReceived;
} 

//closes an RCS socket descriptor
int rcsClose(int socketID) {
    removeConnection(socketID);
	return ucpClose(socketID);
} 


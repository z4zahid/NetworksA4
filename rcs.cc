#include "ucp.c"
#include "rcs.h"

#include <iostream>
#include <vector>

#define SYN_BIT 0
#define ACK_BIT 1
#define SEQ_NUM 2
#define ACK_NUM 3
#define DATA_INDEX 4
#define HEADER_SIZE 5

#define BUFFER_SIZE 517 // 512 + HEADER_SIZE

#define ACK_SET 'a'
#define SYN_SET 's'

using namespace std;

char serverSeq = 0;
char clientSeq = 0;

pthread_mutex_t lock;
int counter;

// vector of sockets
vector<struct Connection> connections;

void addConnection(struct Connection connection) {
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

    // Note to Zainab: there's a case where the final handshake ACK may be lost
    // In that case, server will send us a SYNACK
    // If that's the case, you will need to send the missing ACK as well as the message
    // Feel free to bug me if there's any issues
    return 0;

} 


// blocks sending data. 
// Returns the actual number of bytes sent. If rcsSend() returns with a
// non-negative return value, then we know that so many bytes were reliably received by the other end
int rcsSend(int socketID, const void * sendBuffer, int numBytes) {
    return 0;

} 

//closes an RCS socket descriptor
int rcsClose(int socketID) {
    removeConnection(socketID);
	return ucpClose(socketID);
} 


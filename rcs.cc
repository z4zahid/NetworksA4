#include "ucp.c"
#include "rcs.h"
#include "lib.h"

#include <iostream>
#include <pthread.h>
#include <string>
#include <cstring>
#include <cerrno>

using namespace std;

//extern int errno;

char serverSeq = 0;
char clientSeq = 0;

int counter;

//used to allocate an RCS socket. Returns a socket descriptor (positive integer) on success
int rcsSocket() {
   return ucpSocket();
} 

//binds an RCS socket (first argument) to the address structure (second argument)
int rcsBind(int socketID, struct sockaddr_in * addr) {

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
    return SUCCESS;
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
    	if (receiveBuf[CHK_SUM] == CHK_SET && receiveBuf[CLOSE_BIT] == CLOSE_SET) {
    	  sendBuf[CLOSE_ACK] = CLOSE_SET;
    	  ucpSendTo(socketID, sendBuf, BUFFER_SIZE, ackAddr);
    	  break;
    	} else if (receiveBuf[CHK_SUM] == CHK_SET && receiveBuf[ACK_BIT] == ACK_SET && receiveBuf[ACK_NUM] == seq_num + 1) {
            connection.ack = true;
            break;
        }
    }
    
    return SUCCESS;
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
    
    Connection connection;
    connection.destination = *addr;
    connection.socketID = socketID;
    connection.ack = false;
    connection.ackNum = ack_num;
    addConnection(connection);

    // Send ACK to the server
    ucpSendTo(socketID, buf, BUFFER_SIZE, addr);
    
	//success
    return SUCCESS;
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
        int success = receiveDataPacket(socketID, &packet, &addr);
        
		if (success < 0)
			continue;

		if (!IsPacketCorrupted(packet, expectedBytes, bytesReceived)) {
        
            if (expectedBytes == 0) {
        		bytesReceived = 0;
                expectedBytes = packet.totalBytes;
                expectedPackets = getTotalPackets(expectedBytes);
		cout << "expectedPackets " << expectedPackets << " and bytes " << expectedBytes << endl;
        	    packets.resize(expectedPackets);
            }

            if (packet.sequenceNum >= rcvBase && packet.sequenceNum < expectedPackets){ 

                int ack[2];
                ack[SEQUENCE_NUM] = packet.sequenceNum;
                ack[PACKET_LEN] = packet.packetLen;
                ucpSendTo(socketID, &ack, sizeof(ack), &addr);
                //If the packet was not previously received, it is buffered.
				if (packet.sequenceNum < packets.size() && packets[packet.sequenceNum].sequenceNum < 0) {
                    packets[packet.sequenceNum] = packet;
                    bytesReceived += packet.packetLen;

		    cout << "bytes rcv: " << bytesReceived << " packet " << packet.sequenceNum << endl;
                }

                if (packet.closeBit == CLOSE_SET) {
		       cout << "cloooosing " << endl;
                       char sendBuf[BUFFER_SIZE];
                       sendBuf[CLOSE_ACK] = CLOSE_SET;
                       ucpSendTo(socketID, sendBuf, BUFFER_SIZE, &addr);
                      break;
                } else if (packet.sequenceNum == rcvBase) {
                    // deliver this packet and any previously buffered and consecutively numbered packets
                    int i = packet.sequenceNum, j;
                    while( i< packets.size() && packets[i].sequenceNum >= 0) {

                        DataPacket p = packets[i];
                        int index = p.sequenceNum*MAX_PACKET_SIZE;
                        char* iterate = (char*)rcvBuffer;
						for (j=0; j<index; j++){
                            iterate++;
                        }

                        memcpy(iterate, &p.data, p.packetLen);
                        i++;
                    }
                    // moved receive window forward by the number of packets delivered
                    int moveForwardBy = i - packet.sequenceNum; 
			cout << "new window " << i << endl;
                    rcvBase = i;
                    rcvBaseHi = ((rcvBaseHi + moveForwardBy) > expectedPackets )? expectedPackets: rcvBaseHi + moveForwardBy;
                } 
            } else if (packet.sequenceNum >= (rcvBase - WINDOW_SIZE) && packet.sequenceNum <= (rcvBase - 1)) {
                //return ACK to sender even though we've already ACKED before
                cout << "ACKED before " << packet.sequenceNum << endl;
		int ack[2];
                ack[SEQUENCE_NUM] = packet.sequenceNum;
                ack[PACKET_LEN] = packet.packetLen;
                ucpSendTo(socketID, &ack, sizeof(ack), &addr); 
            } else {
                //ignore
            	cout << "IGNORE " << packet.sequenceNum << endl;
	    }
        }
    }
    return bytesReceived;
} 

/* evilKind == 0 -- send only part of the bytes 
 *             1 -- corrupt some of the bytes
 *             2 -- pretend that we sent all the bytes
 */
// blocks sending data. 
// Returns the actual number of bytes sent. If rcsSend() returns with a
// non-negative return value, then we know that so many bytes were reliably received by the other end
int rcsSend(int socketID, const void * sendBuffer, int numBytes) {
   
	if (numBytes < 1)
		return SEND_ZERO_BYTES;
 
    vector<DataPacket> dataPackets;
    int numPackets = getTotalPackets(numBytes);
	cout << "POPULATE " << endl;
   	populateDataPackets(sendBuffer, numBytes, socketID, &dataPackets);
	cout << "POPULATE OKAY " << endl;
    //let's send out all the packets and then we can wait for them
    int i;
    for (i = 0; i<numPackets;i++) {
		sendDataPacket(socketID, &dataPackets.at(i));
    }

    int rcvPackets[numPackets];
    memset(rcvPackets, 0, numPackets*sizeof(int));
    int retransmits[numPackets];
    memset(retransmits, 0, numPackets*sizeof(int));
    int bytesReceived = 0;
    int curWindowLo = 0;
    int curWindowHi = (numPackets < (WINDOW_SIZE - 1))? numPackets : WINDOW_SIZE-1;
	int packetsReceived = 0;

    //now we receive them and move around our window accordingly
    while (bytesReceived < numBytes && !allRetransmitsTimedOut(retransmits, numPackets)) {
        
		if (retransmits[curWindowLo] == MAX_RETRANSMIT){
			int j = curWindowLo + 1;
			while (j < numPackets && rcvPackets[j] != 0) {
				j++;
			}
			int move = j - curWindowLo;
			curWindowLo = j;
			curWindowHi = ((curWindowHi + move) > numPackets)? numPackets : curWindowHi + move; 
		}

		ucpSetSockRecvTimeout(socketID, ACK_TIMEOUT);

        struct sockaddr_in addr;
        int ack[BUFFER_SIZE];
        ssize_t bytes = ucpRecvFrom(socketID, &ack, sizeof(ack), &addr);
        
        // Check if it's an incomplete SYNACK
        bool retrans = false;
        while (bytes > 0) {
            if (ack[SYN_BIT] == SYN_SET && ack[ACK_BIT] == ACK_SET && ack[CHK_SUM] == CHK_SET && ack[SEQ_NUM] == getConnection(socketID).ackNum + 1 ) {
                // Send the missing ack
                char buf[BUFFER_SIZE];
                memset(buf, 0, BUFFER_SIZE);
                
                buf[ACK_NUM] = getConnection(socketID).ackNum;
                buf[ACK_BIT] = ACK_SET;
                buf[CHK_SUM] = CHK_SET;
                
                // Send ACK to the server
                ucpSendTo(socketID, buf, BUFFER_SIZE, &addr);
                retrans = true;
                bytes = ucpRecvFrom(socketID, &ack, sizeof(ack), &addr);
            } else {
                break;
            }
        }
        
        if (retrans) {
            for (i = 0; i<numPackets;i++) {
                sendDataPacket(socketID, &dataPackets.at(i));
            }
            memset(ack, 0, BUFFER_SIZE);
            bytes = ucpRecvFrom(socketID, &ack, sizeof(ack), &addr);
        }
        
        // ACK received:
        if (bytes > 0) {
            
            if (ack[CLOSE_BIT] == CLOSE_SET) {
                char sendBuf[BUFFER_SIZE];
                sendBuf[CLOSE_ACK] = CLOSE_SET;
                ucpSendTo(socketID, sendBuf, BUFFER_SIZE, &addr);
                break;
            }

            int seq = ack[SEQUENCE_NUM];
            // if in current window, mark packet as received
            if (seq >= curWindowLo && seq < numPackets) {
                if (rcvPackets[seq]== 0 && ack[PACKET_LEN] <= MAX_PACKET_SIZE) {
                    bytesReceived += ack[PACKET_LEN];
                    rcvPackets[seq] = 1;
                }
            } 

            // if this packet's sequence number = send_base
            if (seq == curWindowLo) {
                // case 1: this is the ACK we expect -> move window
                int i = seq;
                while(i< numPackets && rcvPackets[i] != 0) {
                    i++;
                }
                int moveForwardBy = (i - seq); 
                curWindowLo = i;
                curWindowHi = ((curWindowHi + moveForwardBy) > numPackets )? numPackets: curWindowHi + moveForwardBy;
            } else {
                // case 2: this is greater than the ACK we expect -> retransmit ones in middle
                i = curWindowLo;
                if (i < dataPackets.size() && rcvPackets[i] == 0 && retransmits[i] < MAX_RETRANSMIT) {
                    sendDataPacket(socketID, &dataPackets.at(i));
                    retransmits[i] = retransmits[i] + 1;
                }
            }
        } else {
            // case 3: we did not get an ACK back at all -> retransmit the one we're expecting
	    cout << "no ACK " << endl;
            int i = curWindowLo;
            if (i < dataPackets.size() && rcvPackets[i] == 0 && retransmits[i] < MAX_RETRANSMIT ) {
                sendDataPacket(socketID, &dataPackets.at(i));
                retransmits[i] = retransmits[i] + 1;
            }
        }  
    }

    return bytesReceived;
} 

//closes an RCS socket descriptor
int rcsClose(int socketID) {
    // Send message to the socketID
    Connection conn = getConnection(socketID);
    char receiveBuf[BUFFER_SIZE];
 
    char sendBuf[BUFFER_SIZE];
    sendBuf[SYN_BIT] = 0;
    sendBuf[ACK_BIT] = 0;
    sendBuf[CHK_SUM] = CHK_SET;
    sendBuf[CLOSE_BIT] = CLOSE_SET;

    ucpSetSockRecvTimeout(socketID, 1000);
    memset(receiveBuf, 0, BUFFER_SIZE);
    
    struct sockaddr_in ackAddr;
    for (int i = 0; i < MAX_RETRANSMIT; i++) {
	cout << "SEND THE THING " << endl;
        ucpSendTo(socketID, sendBuf, BUFFER_SIZE, &(conn.destination));
        ucpRecvFrom(socketID, receiveBuf, BUFFER_SIZE, &ackAddr);
        if (receiveBuf[CHK_SUM] == CHK_SET && receiveBuf[CLOSE_ACK] == CLOSE_SET) {
            cout << "close been receivied, breaking now" << endl;
	    break;
        }
    }
    cout << "removing connection" << endl;
    removeConnection(socketID);
    cout << "closing connection" << endl;
   int u = ucpClose(socketID);
	cout << "OKAY " << endl;

    return u;
} 


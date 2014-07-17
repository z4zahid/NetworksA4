#include "ucp.c"
#include "rcs.h"

#include <iostream>
#include <pthread.h>
#include <string>
#include <cstring>
#include <cerrno>

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
   	cout << "pushback: " << inet_ntoa(connection.destination.sin_addr) << " scoket " << connection.socketID <<  endl;
    pthread_mutex_unlock(&lock);
}

Connection getConnection(int socketID) {
    pthread_mutex_lock(&lock);
    for (int i = 0 ; i < connections.size(); i++) {
        if ((connections.at(i)).socketID == socketID) {
            return connections.at(i);
        }
    }
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
    cout << "get waiting for lock " << connections.size() << endl;
	 pthread_mutex_lock(&lock);
    for (int i = 0 ; i < connections.size(); i++) {
		cout << "conn socket " << connections.at(i).socketID << endl;
        if ((connections.at(i)).socketID == socketID) {
   			cout << "GET: " << inet_ntoa(connections.at(i).destination.sin_addr) << " scoket " << connections.at(i).socketID <<  endl;
            pthread_mutex_unlock(&lock);
			return connections.at(i).destination;
        }
    }
    pthread_mutex_unlock(&lock);
	cout << "returning with nothing " << endl;
}

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
   			cout << "storing: " << inet_ntoa(addr->sin_addr) << " scoket " << socketID <<  endl;
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
    
    Connection connection;
    connection.destination = *addr;
    connection.socketID = socketID;
    connection.ack = false;
    connection.ackNum = ack_num;
    addConnection(connection);
    cout << "client storing: " << inet_ntoa(addr->sin_addr) << " scoket " << socketID <<  endl;

    // Send ACK to the server
    ucpSendTo(socketID, buf, BUFFER_SIZE, addr);
    
	//success
    return 0;
}

int receiveDataPacket(int socketID, DataPacket *packet, struct sockaddr_in* addr) {

    int size = MAX_PACKET_SIZE + 16;
    char data[size]; 
    memset(data, 0, size);
    int bytes = ucpRecvFrom(socketID, data, size, addr);
	cout << "bytes rcv: " << bytes << endl;
	if (bytes < 0)
		return -1;

	cout << errno << " " <<  strerror(errno) << endl;

    memcpy(&packet->sequenceNum, &data[0], sizeof(int));
    cout << "rcv: sequenceNum: " << packet->sequenceNum << endl;
    memcpy(&packet->totalBytes, &data[4], sizeof(int));
    cout << "rcv: totalBytes: " << packet->totalBytes << endl;
    memcpy(&packet->checksum, &data[8], sizeof(int));
    cout << "rcv: checksum: " << packet->checksum << endl;
    memcpy(&packet->packetLen, &data[12], sizeof(int));
    cout << "rcv: len: " << packet->packetLen << endl;
	if (packet->packetLen > 0 && packet->packetLen <= MAX_PACKET_SIZE)
    	memcpy(&packet->data, &data[16], packet->packetLen);
	
	return 0;

}

void sendDataPacket(int socketID, DataPacket *packet) {

    int size = packet->packetLen + 16; //4 ints to be stored as chars
	sockaddr_in addr = getConnectionAddr(socketID);
   	cout << "sending to: " << inet_ntoa(addr.sin_addr) <<" socket " << socketID << endl;
    ucpSendTest(socketID, packet->data, size, &addr);
}

int getTotalPackets(int numBytes) {
    int numPackets = numBytes/MAX_PACKET_SIZE;
    if (numBytes % MAX_PACKET_SIZE > 0) {
        numPackets++;
    }
    cout << "totalPackets: " << numPackets << endl;
    return numPackets;
}
    
// let's just sum them, ucpSend corrupts each packet, very unlikely this hash will fail
int getChecksum(const void* packet, int size) {

	if (size <0 || size > MAX_PACKET_SIZE)
		return -1;

    int sum = 0, i;
    char* it = (char*)packet;
    for (i = 0; i< size; i++) {
        sum += (int)(*it);
        it++;
    }
	cout << "sum " << sum << endl;
    return sum;
}

int IsPacketCorrupted(DataPacket packet) {
	
	if (packet.packetLen < 0) {
		cout << "bad length" << endl;
		return 1;
	}

	cout << "checksum " << packet.checksum << endl;
    //checksum
    if (getChecksum(packet.data, packet.packetLen) != packet.checksum) {
        cout << "corrupted! checksum" << endl;
        return 1;
    }

    /*data length
    int numBytes = 0;
    char* it = (char*)packet.data;
    while(it) {
        numBytes++;
        it++;
    }
	cout << "length: " << numBytes << endl;
    if (numBytes != packet.packetLen) {
        cout << "corrupted! length" << endl;
        return 1;
    }*/

    cout << "clean packet" << endl;
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

    cout << "start rcv" << endl;
    while (bytesReceived < expectedBytes && bytesReceived < maxBytes) {
        
        DataPacket packet;
        struct sockaddr_in addr;
        int success = receiveDataPacket(socketID, &packet, &addr);
        
		if (success < 0)
			continue;
        /*if (expectedBytes == 0) {
			bytesReceived = 0;
            expectedBytes = packet.totalBytes;
            expectedPackets = getTotalPackets(expectedBytes);
            packets.resize(expectedPackets);
        }*/
        
        cout << "checking corrupted packet maxBytes " << maxBytes <<  endl;
		if (!IsPacketCorrupted(packet)) {
        if (expectedBytes == 0) {
			bytesReceived = 0;
            expectedBytes = packet.totalBytes;
            expectedPackets = getTotalPackets(expectedBytes);
            packets.resize(expectedPackets);
        }
            if (packet.sequenceNum >= rcvBase && packet.sequenceNum <= rcvBaseHi) {

                int ack[2];
                ack[SEQUENCE_NUM] = packet.sequenceNum;
                ack[PACKET_LEN] = packet.packetLen;
                cout << "rcv: ACK" << endl;
                ucpSendTest(socketID, &ack, sizeof(ack), &addr);

                //If the packet was not previously received, it is buffered.
                cout << "pack.seq " << packet.sequenceNum << " size" << packets.size() << endl;
				if (packets[packet.sequenceNum].sequenceNum < 0) {
                    packets[packet.sequenceNum] = packet;
                    bytesReceived += packet.packetLen;
                    cout << "bytesReceived: " << bytesReceived << endl;
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

                        cout << "copy to rcv buffer " << endl;
                        memcpy(iterate, &p.data[16], p.packetLen);
                    
                        i++;
                    }
                    // moved receive window forward by the number of packets delivered
                    int moveForwardBy = i - packet.sequenceNum; 
                    rcvBase += moveForwardBy;
                    rcvBaseHi = ((rcvBaseHi + moveForwardBy) > expectedPackets )? expectedPackets: rcvBaseHi + moveForwardBy;
                } 
            } else if (packet.sequenceNum >= (rcvBase - WINDOW_SIZE) && packet.sequenceNum <= (rcvBase - 1)) {
                //return ACK to sender even though we've already ACKED before
                int ack[2];
                ack[SEQUENCE_NUM] = packet.sequenceNum;
                ack[PACKET_LEN] = packet.packetLen;
                cout << "rcv: already ACKED" << endl;
                ucpSendTest(socketID, &ack, sizeof(ack), &addr); 
            } else {
                //ignore
                cout << "ignore: " << packet.sequenceNum << endl;
            }
        }
		cout << "calling receive again " << endl;
    }
	cout << "complete: bytesReceived " << bytesReceived << endl;
    return bytesReceived;
} 

int allRetransmitsTimedOut(int retransmits[], int size) {
    int i;
    for ( i=0; i<size; i++) {
        if (retransmits[i] < MAX_RETRANSMIT){
            return 0; 
 	   }
	}
    return 1;
}

void populateDataPackets(const void* sendBuffer, int numBytes, int socketID, vector<DataPacket>* packets) {

    int i = 0;
    int numPackets = getTotalPackets(numBytes);

    for (i = 0; i<numPackets;i++) {
        
        DataPacket packet;
        packet.sequenceNum = i;
        packet.totalBytes = numBytes;
        packet.packetLen = (i == numPackets - 1)? (numBytes - MAX_PACKET_SIZE*i) : MAX_PACKET_SIZE;

        int size = packet.packetLen + 16; //4 ints to be stored as chars
        memset(packet.data, 0, size);

        int index = i*MAX_PACKET_SIZE;
        char* iterate = (char*)sendBuffer;
        for (i=0; i<index; i++){
            iterate++;
        }

        packet.checksum = getChecksum(iterate, packet.packetLen);

        memcpy(&packet.data[0], &packet.sequenceNum, sizeof(int));
        int seq;
        memcpy(&seq, &packet.data[0], sizeof(int));
        cout << "sequenceNum " << packet.sequenceNum << " copied: " << seq << endl;

        memcpy(&packet.data[4], &packet.totalBytes, sizeof(int));
        int total;
        memcpy(&total, &packet.data[4], sizeof(int));
        cout << "totalBytes " << packet.totalBytes << " copied: " << total << endl;

        memcpy(&packet.data[8], &packet.checksum, sizeof(int));
        int check;
        memcpy(&check, &packet.data[8], sizeof(int));
        cout << "checksum " << packet.checksum << " copied: " << check << endl;

        memcpy(&packet.data[12], &packet.packetLen, sizeof(int));
        int len;
        memcpy(&len, &packet.data[12], sizeof(int));
        cout << "packetLen " << packet.packetLen << " copied: " << len << endl;

        memcpy(&packet.data[16], iterate, packet.packetLen);
        
        packets->push_back(packet);
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
        sendDataPacket(socketID, &dataPackets.at(i));
        curSequenceNum++;
    }

    int rcvPackets[numPackets];
    memset(rcvPackets, 0, numPackets*sizeof(int));
    int retransmits[numPackets];
    memset(retransmits, 0, numPackets*sizeof(int));
    int bytesReceived = 0;
    int curWindowLo = 0;
    int curWindowHi = (numPackets < (WINDOW_SIZE - 1))? numPackets : WINDOW_SIZE-1;
	int packetsReceived = 0;

    cout << "ACK rcv start" << endl;
    //now we receive them and move around our window accordingly
    while (bytesReceived < numBytes && !allRetransmitsTimedOut(retransmits, numPackets)) {

        ucpSetSockRecvTimeout(socketID, ACK_TIMEOUT);

        struct sockaddr_in addr;
        int ack[2];
        ssize_t bytes = ucpRecvFrom(socketID, &ack, sizeof(ack), &addr);
        
        // ACK received:
        if (bytes > 0) {
            
            int seq = ack[SEQUENCE_NUM];
            cout << "ACK: " << seq << endl;

            // if in current window, mark packet as received
            if (seq >= curWindowLo && seq <= curWindowHi) {
                if (rcvPackets[seq]== 0 && ack[PACKET_LEN] <= MAX_PACKET_SIZE) {
                    bytesReceived += ack[PACKET_LEN];
                    cout << "bytesReceived: " << bytesReceived << endl;
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

                int moveForwardBy = i - seq; 
                curWindowLo += moveForwardBy;
                cout << "exact ACK new window lo " << curWindowLo << " + " << moveForwardBy <<  endl;
                curWindowHi = ((curWindowHi + moveForwardBy) > numPackets )? numPackets: curWindowHi + moveForwardBy;

            } else {
                // case 2: this is greater than the ACK we expect -> retransmit ones in middle
                cout << "expected ACK" << curWindowLo << " seq " << seq << endl;
                i = curWindowLo;
				//for (i = curWindowLo; i<seq;i++) {
                    if (rcvPackets[i] == 0 && retransmits[i] < MAX_RETRANSMIT) {
						sendDataPacket(socketID, &dataPackets.at(i));
                        retransmits[i] = retransmits[i] + 1;
                    }
                //}
            }
        }
        else {
            // case 3: we did not get an ACK back at all -> retransmit the one we're expecting
            int i = curWindowLo;
            cout << "no ACK " <<  endl;
            if (rcvPackets[i] == 0 && retransmits[i] < MAX_RETRANSMIT) {
               cout << "rexmitting " << i << " times " << retransmits[i] << endl;
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
    
    struct sockaddr_in * ackAddr;
    for (int i = 0; i < MAX_RETRANSMIT; i++) {
        ucpSendTo(socketID, sendBuf, BUFFER_SIZE, &(conn.destination));
        ucpRecvFrom(socketID, receiveBuf, BUFFER_SIZE, ackAddr);
        if (receiveBuf[CHK_SUM] == CHK_SET && receiveBuf[CLOSE_ACK] == CLOSE_SET) {
            break;
        }
    }
    removeConnection(socketID);

    return ucpClose(socketID);
} 


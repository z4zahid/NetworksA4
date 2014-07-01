#include <iostream>

using namespace std;

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
    return ucpBind(socketID, addr)
}

//fills address information into the second argument with which an RCS socket (first argument) has been 
//bound via a call to rcsBind()
int rcsGetSockName(int socketID, struct sockaddr_in * addr){

	//success
	return 0;
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
int rcsAccept(int socketID, struct sockaddr_in * addr) {

}

// connects a client to a server. The socket (first argument) must have been bound beforehand using rcsBind().
// The second argument identifies the server to which connection should be attempted
int rcsConnect(int socketID, const struct sockaddr_in * addr) {

	//success
	return 0;
}


// blocks awaiting data on a socket (first argument).
// Returns the actual amount of data received. “Amount” is the number of bytes. 
// Data is sent and received reliably, so any byte that is
// returned by this call should be what was sent, and in the correct order.
int rcsRecv(int socketID, void * rcvBuffer; int maxBytes) {


} 


// blocks sending data. 
// Returns the actual number of bytes sent. If rcsSend() returns with a
// non-negative return value, then we know that so many bytes were reliably received by the other end
int rcsSend(int socketID, const void * sendBuffer; int numBytes) {


} 

//closes an RCS socket descriptor
int rcsClose(int socketID) {

	//success
	return 0;
} 


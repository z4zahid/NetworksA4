#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


struct Connection {
	sockaddr_in destination;
    int socketID;
	bool ack;
};

int rcsSocket();

int rcsBind(int socketID, const struct sockaddr_in * addr);


int rcsGetSockName(int socketID, struct sockaddr_in * addr);

int rcsListen(int socketID);

int rcsAccept(int socketID, struct sockaddr_in *addr); 

int rcsConnect(int socketID, const struct sockaddr_in * addr); 

int rcsRecv(int socketID, void * rcvBuffer, int maxBytes);

int rcsSend(int socketID, const void * sendBuffer, int numBytes);

int rcsClose(int socketID);


 






 
 


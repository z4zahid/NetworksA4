#ifndef _RCS_
#define _RCS_

#include <vector>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#include "ucp.c"

typedef struct conn {
    struct sockaddr_in destination;
    int socketID;
    bool ack;
    int ackNum;
} Connection;

int rcsSocket();
int rcsBind(int socketID, struct sockaddr_in * addr);
int rcsGetSockName(int socketID, struct sockaddr_in * addr);
int rcsListen(int socketID);
int rcsAccept(int socketID, struct sockaddr_in *addr);
int rcsConnect(int socketID, const struct sockaddr_in * addr);
int rcsRecv(int socketID, void * rcvBuffer, int maxBytes);
int rcsSend(int socketID, const void * sendBuffer, int numBytes);
int rcsClose(int socketID);
struct sockaddr_in getConnectionAddr(int socketID);
#endif


#ifndef _RCS_
#define _RCS_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <vector>



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


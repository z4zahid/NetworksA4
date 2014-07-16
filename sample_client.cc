#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include "rcs.h"

static const int kBufferLength = 20;

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Error, enter server name and port"); 
    }

    int server_port = 0;
    int client_socket = rcsSocket();

    struct sockaddr_in server_addr;
    
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = 0;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if(rcsBind(client_socket, (struct sockaddr_in *)(&server_addr)) < 0) {
        perror("bind"); exit(1);
    }

    cout << "Finish binding" << endl;

    // Lookup address provided.
    // The following piece of code was written after consulting the code shown in tutorial.
    struct addrinfo *res, hints; 
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(argv[1], NULL, (const struct addrinfo*)(&hints), &res) != 0 ) {
        perror("getaddrinfo");
        return 0;
    }

    struct addrinfo *list_iterator;
    for (list_iterator = res; list_iterator != NULL; list_iterator = list_iterator->ai_next) {
        struct in_addr addr;
        if (list_iterator->ai_family == AF_INET) {
            server_addr.sin_addr.s_addr = ((struct sockaddr_in *) (list_iterator->ai_addr)) ->sin_addr.s_addr;
            break;
        }
    }
    rcsConnect(client_socket, &server_addr);

    cout << "Starting Send/Receive testing" << endl;

    //string s = "bah";
   // rcsSend(client_socket, s.c_str(), s.length());
    //ucpSendTo(client_socket, s.c_str(), s.length() + 1, &server_addr);

    // Connedct to a server.
  /*  rcsBind(client_socket, (const struct sockaddr_in *) &server_addr);

    if (rcsConnect(client_socket, (struct sockaddr_in *) &server_addr) != 0) {
        cerr<<"Unable to connect to the server"<<endl;
        exit(1);
    }*/
    return 0;
}


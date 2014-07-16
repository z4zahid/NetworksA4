#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sstream>

#include "rcs.h"

using namespace std;

int main(int arcg, char* argv[]) {
    int server_port = 0;
    int server_socket = -1; 
    // Get a socket
    while (server_socket < 0) {
        server_socket = rcsSocket();
        // server_socket = rcsSocket();
    }
    cout<<"Server socket"<<server_socket<<endl;
    struct sockaddr_in addr, client_addr;
    memset(&addr, 0, sizeof( &addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rcsBind(server_socket, &addr);
    //cout<<bind(server_socket, (const struct sockaddr *) &addr, (socklen_t)sizeof(struct sockaddr_in))<<endl;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    socklen_t addrlen = sizeof(struct sockaddr_in);
    getsockname(server_socket, (struct sockaddr *) &addr, &addrlen);
    // Get the external IP address of the 
    // http://www.linuxhowtos.org/manpages/3/getifaddrs.htm
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    int s, family;
    if (getifaddrs(&ifaddr) < 0) {
        perror ("getifaddrs");
        return 0;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        family = ifa->ifa_addr->sa_family;

        // Print the ip address of the server
        if (strcmp(ifa->ifa_name, "eth0") == 0 && family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                perror("getnameinfo()");
                return 0;
            }
            printf("%s ", host);
        }
    }
    freeifaddrs(ifaddr);

    // Print the IP port number
    cout << ntohs(addr.sin_port) << endl;

    char buffer[1024];
    addrlen = (socklen_t)sizeof(struct sockaddr_in);
    
    rcsListen(server_socket);

    cout << "Listen fini" << endl;

    rcsAccept(server_socket, &addr);
	cout << "server addr: " << inet_ntoa(addr.sin_addr) << " server socket " << server_socket<< endl;
    cout << "Acceptance - Starting Send/Receive testing" << endl;

    rcsRecv(server_socket, &buffer,20 ); 
    //recvfrom(server_socket, buffer, 1024, 0, (struct sockaddr *)&addr, &addrlen);
    //cout<<buffer<<endl;
    /* 
    // Bind to a port. This call is the function provided with the assignment by Prof Tripunitara.
    cout<<"rcs call"<<rcsBind(server_socket, &addr);
    // Get the external IP address of the 
    // http://www.linuxhowtos.org/manpages/3/getifaddrs.htm
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    int s, family;
    if (getifaddrs(&ifaddr) < 0) {
    perror ("getifaddrs");
    return 0;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL) {
    continue;
    }
    family = ifa->ifa_addr->sa_family;

    // Print the ip address of the server
    if (strcmp(ifa->ifa_name, "eth0") == 0 && family == AF_INET) {
    s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    if (s != 0) {
    perror("getnameinfo()");
    return 0;
    }
    printf("%s ", host);
    }
    }
    freeifaddrs(ifaddr);    

    cout<<ntohs(addr.sin_port)<<endl; 


    // Waiting for clients. This server binds to the first client that comes in when it's free
    // and will be dedicated to that client until a STOP_SESSION or a STOP message is sent.
    while (1) {
    rcsListen(server_socket);
    socklen_t client_addrSize = sizeof(client_addr);
    int new_socket = rcsAccept(server_socket, (const struct sockaddr_in *) &client_addr);  
    }*/
    return 0; 
}


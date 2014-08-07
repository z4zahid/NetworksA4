/* Mahesh V. Tripunitara
 * University of Waterloo
 * tcp-client.c -- takes as cmd line args a server ip & port.
 * After establishing a connection, reads from stdin till eof
 * and sends everything it reads to the server. sleep()'s
 * occasionally in the midst of reading & sending.
 */

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int rcsSocket();
extern int rcsBind(int, struct sockaddr_in *);
extern int rcsGetSockName(int, struct sockaddr_in *);
extern int rcsListen(int);
extern int rcsAccept (int, struct sockaddr_in *);
extern int rcsConnect (int, const struct sockaddr_in *);
extern int rcsRecv (int, void *, int);
extern int rcsSend(int, const void *, int);
extern int rcsClose(int);

unsigned int getrand() {
    int f = open("/dev/urandom", O_RDONLY);
    if(f < 0) {
        perror("open(/dev/urandom)"); return 0;
    }
    
    unsigned int ret;
    read(f, &ret, sizeof(unsigned int));
    close(f);
    return ret;
}

int main(int argc, char *argv[]) {
	int i,j;
	int received;
	    FILE *f;
	    char ip[20];
	    char port[10];
		char *fname;
		int start;
    
	    while(1) {
	        f = fopen("port","r");
	        if (f != NULL)
	            break;
	    }
    
	    printf("Opened port file\n");
    
	    fgets(ip,20,f);
	    fgets(port,10,f);
	
		printf("%s %u\n",ip,atoi(port));
    
	    fclose(f);
    
	    int s = rcsSocket();
	    struct sockaddr_in a;
    
	    memset(&a, 0, sizeof(struct sockaddr_in));
	    a.sin_family = AF_INET;
	    a.sin_port = 0;
	    a.sin_addr.s_addr = INADDR_ANY;
    
	    if(rcsBind(s, (struct sockaddr_in *)(&a)) < 0) {
	        perror("bind"); exit(1);
	    }

	    if(rcsGetSockName(s, &a) < 0) {
		fprintf(stderr, "rcsGetSockName() failed. Exiting...\n");
	        exit(0);
	    }
    
	    unsigned char buf[256];
	    int nread = -1;
    
	    a.sin_family = AF_INET;
	    a.sin_port = htons((uint16_t)(atoi(port)));
	    if(inet_aton(ip, &(a.sin_addr)) < 0) {
	        fprintf(stderr, "inet_aton(%s) failed.\n", ip);
	        exit(1);
	    }
    
	    printf("Will connect\n");
    
	    if(rcsConnect(s, (struct sockaddr_in *)(&a)) < 0) {
	        perror("connect"); exit(1);
	    }
    
	    printf("Connected\n");
    
    fname = (char*)malloc(1024*10*sizeof(char));
    memset(fname, 0, 10*1024);
	start = 0;
   
	while (start < 10*1024) {
		received = rcsRecv(s,fname+start,10*1024-start);
				
		start += received;
	}
	printf("Received\n");
	
	
	for (i = 0; i < received; i++) {
		if (fname[i] != i%2)
			system("echo Failed2 > Result2c");
	}
	printf("Verified\n");
			
	system("echo Success2 > Result2c");
    
    rcsClose(s);
    
    return 0;
}

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


void recvbytes(int s, void *buf, ssize_t count) {
    /* Recv until we hit count bytes */
    int stillToRecv;
    for(stillToRecv = count; stillToRecv > 0; ) {
	ssize_t recvSize =
	    rcsRecv(s, (void *)(((unsigned char *)buf) + count - stillToRecv), stillToRecv);

	if(recvSize <= 0) {
			system("echo Failed1 > Result1c");
			exit(0);
	}

#ifdef _DEBUG_1_
	printf("recvbytes():\n");
	printBuf((char *)buf + count - stillToRecv, recvSize);
	fflush(stdout);
#endif

	stillToRecv -= recvSize;
    }
}

int main(int argc, char *argv[]) {
int i,j;
int received;
    FILE *f;
    char fname[64];
    char ip[20];
    char port[10];
    
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
    memset(fname, 0, 64);
   
   	for (i = 0; i < 1*1024/64; i++) {
    memset(fname, 0, 64);
		
		recvbytes(s,fname,64);
        printf("Received chunk\n");
		
		for (j = 0; j<64;j++) {
			if (fname[j] != j) {
				system("echo Failed1 > Result1c");
				exit(0);
			}
        }
        printf("Verified chunk\n");
   	}
			
	system("echo Success1 > Result1c");
    
    rcsClose(s);
    
    return 0;
}

/* Mahesh V. Tripunitara
 * University of Waterloo
 * tcp-server.c -- first prints out the IP & port on which in runs.
 * Then awaits connections from clients. Create a pthread for each
 * connection. The thread just reads what the other end sends and
 * write()'s it to a file whose name is the thread's ID.
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
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define _DEBUG_


extern int rcsSocket();
extern int rcsBind(int, struct sockaddr_in *);
extern int rcsGetSockName(int, struct sockaddr_in *);
extern int rcsListen(int);
extern int rcsAccept (int, struct sockaddr_in *);
extern int rcsConnect (int, const struct sockaddr_in *);
extern int rcsRecv (int, void *, int);
extern int rcsSend(int, const void *, int);
extern int rcsClose(int);

void sendbytes(int s, void *buf, ssize_t count) {
    int stilltosend;
    for(stilltosend = count; stilltosend > 0; ) {
	ssize_t sentsize =
	    rcsSend(s, (void *)(((unsigned char *)buf) + count - stilltosend), stilltosend);
	if(sentsize < 0) {
			system("echo Failed0 > Result0s");
			exit(0);
	}

#ifdef _DEBUG_1_
	printf("sendbytes():\n");
	printBuf((char *)buf + count - stilltosend, sentsize);
	fflush(stdout);
#endif
	stilltosend -= sentsize;
    }
}


void *serviceConnection(void *arg) {
    int s = *(int *)arg;
    int i,j;
    int received;
    int sent;
    free(arg);
    char fname[64];
    memset(fname, 0, 64);
   
    printf("Server beginning\n");
   	for (i = 0; i < 1*1024/64; i++) {
		
		for (j = 0; j<64;j++) {
			fname[j] = j;
		}
   
	    printf("Sending chunk\n");
   		sendbytes(s,fname,64);
   	}
			
	system("echo Success0 > Result0s");
   	
	rcsClose(s);
        return NULL;
    }
    
    uint32_t getPublicIPAddr() {
        struct ifaddrs *ifa;
        
        if(getifaddrs(&ifa) < 0) {
            perror("getifaddrs"); exit(0);
        }
        
        struct ifaddrs *c;
        for(c = ifa; c != NULL; c = c->ifa_next) {
            if(c->ifa_addr == NULL) continue;
            if(c->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in a;
                
                memcpy(&a, (c->ifa_addr), sizeof(struct sockaddr_in));
                char *as = inet_ntoa(a.sin_addr);
                //printf("%s\n", as);
                
                int apart;
                sscanf(as, "%d", &apart);
                if(apart > 0 && apart != 127) {
                    freeifaddrs(ifa);
                    return (a.sin_addr.s_addr);
                }
            }
        }
        
        freeifaddrs(ifa);
        return 0;
    }
    
    int main(int argc, char *argv[]) {
        int s = rcsSocket();
        struct sockaddr_in a;
    int i,j;
        FILE *f;
    char fname[64];
        
        memset(&a, 0, sizeof(struct sockaddr_in));
        a.sin_family = AF_INET;
        a.sin_port = 0;
        if((a.sin_addr.s_addr = getPublicIPAddr()) == 0) {
            fprintf(stderr, "Could not get public ip address. Exiting...\n");
            exit(0);
        }
        
        if(rcsBind(s, &a) < 0) {
            fprintf(stderr, "rcsBind() failed. Exiting...\n");
            exit(0);
        }

	if(rcsGetSockName(s, &a) < 0) {
            fprintf(stderr, "rcsGetSockName() failed. Exiting...\n");
            exit(0);
	}
        
        printf("%s %u\n", inet_ntoa(a.sin_addr), ntohs(a.sin_port));
        f = fopen("port","w");
        fprintf(f,"%s\n", inet_ntoa(a.sin_addr));
        fprintf(f,"%u\n", ntohs(a.sin_port));
        fclose(f);
        
        if(rcsListen(s) < 0) {
            perror("listen"); exit(0);
        }
        
        memset(&a, 0, sizeof(struct sockaddr_in));
        int asock;
		printf("Will accept\n");
        while((asock = rcsAccept(s, (struct sockaddr_in *)&a)) > 0) {
            printf("Received connection\n");
            
		   	for (i = 0; i < 10; i++) {
				fname[i] = i;
		   	}
			
			sleep(4);
   
			    printf("Sending 1 chunk\n");
		   		sendbytes(asock,fname,10);
			
				system("echo Success0 > Result0s");
   	
				rcsClose(s);
				exit(0);
        }
        
        return 0;
    }

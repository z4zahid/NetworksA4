# makefile must have the following two targets:
# clean = remove all .o, executable and .a files
# librcs.a. archive file that has all the RCS functions

XCC = g++
CFLAGS  = -c
SOURCES = mybind.c ucp.c rcs.cc

all: librcs.a test rcstest

librcs.a: rcs.o
	ar -rcs librcs.a rcs.o

test: client server

client: rcs.o sample_client.o
	$(XCC) rcs.o sample_client.o -o tc

sample_client.o: sample_client.cc
	$(XCC) $(CFLAGS) sample_client.cc

server: rcs.o sample_server.o
	$(XCC) rcs.o sample_server.o -o ts

sample_server.o: sample_server.cc
	$(XCC) $(CFLAGS) sample_server.cc

rcstest: rcs_client rcs_server

rcs_client: rcs.o rcs_client.o
	$(XCC) rcs.o rcs_client.o -o rcsc -lpthread

rcs_client.o: rcs_client.c
	$(XCC) $(CFLAGS) rcs_client.c

rcs_server: rcs.o rcs_server.o
	$(XCC) rcs.o rcs_server.o -o rcss -lpthread

rcs_server.o: rcs_server.c
	$(XCC) $(CFLAGS) rcs_server.c



clean:
	-rm -f *.o *.a

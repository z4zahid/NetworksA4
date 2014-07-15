# makefile must have the following two targets:
# clean = remove all .o, executable and .a files
# librcs.a. archive file that has all the RCS functions

XCC = g++
CFLAGS  = -c
SOURCES = mybind.c ucp.c rcs.cc

all: librcs.a test

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

clean:
	-rm -f *.o *.a

# makefile must have the following two targets:
# clean = remove all .o, executable and .a files
# librcs.a. archive file that has all the RCS functions

XCC = g++
CFLAGS  = -c
SOURCES = ucp_given.c rcs.cc

librcs.a: ucp_given.o rcs.o
	ar -rcs librcs.a ucp_given.o rcs.o

clean:
	-rm -f *.o *.a
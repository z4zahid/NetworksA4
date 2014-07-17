CXX = g++								# compiler. TODO confirm that g++ runs on ecelinux
CXXFLAGS = -g -Wall -Wno-unused-label -MMD	# compiler flags

LIB_FILES = mybind.c ucp.c rcs.cc
LIB_OBJS = mybind.o ucp.o rcs.o
LIB = librcs.a
EXECS = ${LIB} rcsc rcss



#############################################################

.PHONY : clean

all: librcs.a rcs_client rcs_server

librcs.a : ${LIB_FILES}
	${CXX} -c $^  
	ar -rcs ${LIB} ${LIB_OBJS}
	
###########tests
	
rcsapp : clean librcs.a rcs_client rcs_server
	
rcs_client: rcs_client.c
	${CXX}  -o rcsc $^ -L. -lrcs -lpthread 
	
rcs_server: rcs_server.c
	${CXX}  -o rcss $^ -L. -lrcs -lpthread

###########objs
	
mybind.o: mybind.c
	${CXX} -c $^

ucp.o : ucp.c 
	${CXX} -c $^

rcs.o: rcs.cc 
	${CXX} -c $^
	
clean :	
	rm -f *.d *.o ${EXECS}


	




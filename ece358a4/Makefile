CXX = g++								# compiler. TODO confirm that g++ runs on ecelinux
CXXFLAGS = -g -Wall -Wno-unused-label -MMD	# compiler flags

LIB_FILES = mybind.c ucp.c lib.cc rcs.cc
LIB_OBJS = mybind.o ucp.o lib.o rcs.o
LIB = librcs.a
EXECS = ${LIB} rcsapp-client rcsapp-server



#############################################################

.PHONY : clean

all: librcs.a rcs_client rcs_server

librcs.a : ${LIB_FILES}
	${CXX} -c $^  
	ar -rcs ${LIB} ${LIB_OBJS}
	
###########tests
	
rcsapp : clean librcs.a rcs_client rcs_server
	
rcs_client: rcsapp-client.c
	${CXX}  -o rcsapp-client $^ -L. -lrcs -lpthread 
	
rcs_server: rcsapp-server.c
	${CXX}  -o rcsapp-server $^ -L. -lrcs -lpthread

###########objs

lib.o: lib.c
	${CXX} -c $^

mybind.o: mybind.c
	${CXX} -c $^

ucp.o : ucp.c 
	${CXX} -c $^

rcs.o: rcs.cc 
	${CXX} -c $^
	
clean :	
	rm -f *.d *.o ${EXECS}


	




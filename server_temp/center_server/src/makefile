CC= gcc
CPP = g++
DEBUG  = -g -D_REENTRANT -DDEBUG -Wno-write-strings -w

INCLUDES = -I. -I../../common_lib -I../../aes_lib

LIBS = $(OS_LIB) ../../aes_lib/libaes.a ../../common_lib/libcommon.a -lpthread -lrt -lnsl -lz -lgcc_s -lstdc++ -lcrypto


OBJS = main.o AssignTableHandler.o ClientAuthenHandler.o CommonThread.o EpollSockThread.o RoomAndServerLogic.o ServerEpollSockThread.o ServerLogicThread.o Util.o 
PROG = center_server

CFLAGS = $(DEBUG) $(INCLUDES)
CPPFLAGS = $(DEBUG) $(INCLUDES)


.PHONY : all clean

all:$(PROG)

$(PROG):$(OBJS)
	$(CPP) $(CPPFLAGS) -o ../bin/$(PROG) $(OBJS) $(LIBS)

clean:
	rm -f  *.o *.bak *.lis ../bin/core.* ../bin/$(PROG)
	chmod u-x *.cpp *.h makefile

        

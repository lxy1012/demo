CC= gcc
CPP = g++
DEBUG  = -O0 -w -ggdb -std=c++11 -DGCC_BUILD -DLINUX -D_DEBUG

INCLUDES = -I. -I../../common_lib  -I../../aes_lib  -I../../mysql-8.0.13/include 

LIBS = $(OS_LIB) ../../aes_lib/libaes.a  	$(OS_LIB) ../../common_lib/libcommon.a $(OS_LIB) ../../mysql-8.0.13/lib/libmysqlclient.a \
  -lpthread -lrt -lnsl -lz -lgcc_s -lcrypto -ldl -lssl 

OBJS   = MySQLConnection.o  MySQLTableStruct.o SQLWrapper.o EpollSockThread.o  main.o ServerLogicThread.o Util.o Dispatcher.o AES.o BASE64.o ConfUtil.o
PROG   = radius_server

CFLAGS = $(DEBUG) $(INCLUDES) 
CPPFLAGS = $(DEBUG) $(INCLUDES)

.PHONY : all clean

all:$(PROG)

$(PROG):$(OBJS)
	$(CPP) $(CPPFLAGS) -o ../bin/$(PROG) $(OBJS) $(LIBS)


clean:
	cp -f ../../mysql_com/* ./
	rm -f *.o
	rm -f *.a
	rm -f ../bin/$(PROG)
	rm -f core.*
	rm -f *.bak
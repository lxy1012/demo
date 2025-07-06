CC= gcc #-Wno-deprecated
CXX = g++ #-Wno-deprecated
DEBUG  =  -g 
INCLUDES = -I.

SRCS = aescrypt.c aeskey.c aeslib.c aestab.c
OBJS = aescrypt.o aeskey.o aeslib.o aestab.o

CFLAGS = -D_REENTRANT -DHAVE_DEV_URANDOM $(DEBUG) $(INCLUDES)
CPPFLAGS = -D_REENTRANT -DHAVE_DEV_URANDOM $(DEBUG) $(INCLUDES)

all:libaes.a

libaes.a:$(OBJS) 
	ar cruv libaes.a $(OBJS) 
	ranlib libaes.a

clean:
	rm -f $(OBJS)
	rm -f libaes.a

install:
	cp *.h ../../server/aes_lib
	cp libaes.a ../../server/aes_lib
	cp doc/history.txt ../../server/aes_lib/doc
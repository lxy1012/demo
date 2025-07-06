CC= gcc

CPP = g++

DEBUG  = -g -D_REENTRANT -DDEBUG -DSQLCA_NONE -w 



INCLUDES = -I. -I../../game_factory_src -I../../common_lib -I../../aes_lib 



LIBS =$(OS_LIB) ../../game_factory_src/sinogamefactory.a ../../aes_lib/libaes.a ../../common_lib/libcommon.a  -lpthread -lrt -lnsl -lz -lgcc_s -lcrypto



OBJS   = main.o hzxlmj_gamelogic.o judgecard.o  RandKiss.o GameConf.o  hzxlmj_GameAlgorithm.o hzxlmj_AIControl.o

PROG   = hzxlmj_server



CFLAGS = $(DEBUG) $(INCLUDES)

CPPFLAGS = $(DEBUG) $(INCLUDES) 



.PHONY : all clean



all:$(PROG)



$(PROG):$(OBJS)

	$(CPP) $(CPPFLAGS) -o ../bin/$(PROG) $(OBJS) $(LIBS)







clean:

	rm -f *.o

	rm -f *.a

	rm -f *.bak

	rm -f ../bin/$(PROG)

	rm -f ../bin/core.*

	rm -f ../bin/*.bak

	rm -f *.tmp



	

	
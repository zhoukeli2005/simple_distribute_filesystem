# inherit CC CFLAGS LFLAGS AR NETLIB LIBPATH from parent Makefile

OUTPATH = ../$(LIBPATH)

CFLAGS += -I../include/

OBJS = s_core.o	s_core_param.o	s_core_pkt.o	\
       s_core_create_client.o s_core_create_mserv.o s_core_create_dserv.o	\
       s_core_util.o

.PHONY: all clean

all: $(OBJS)
	@mv $(OBJS) $(OUTPATH)
	

clean:
	@rm -rf $(OBJS)

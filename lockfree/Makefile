# inherit CC CFLAGS LFLAGS AR NETLIB LIBPATH from parent Makefile

OUTPATH = ../$(LIBPATH)

CFLAGS += -I../include/

OBJS = s_ms_queue.o

.PHONY: all clean

all: $(OBJS)
	@cp $(OBJS) $(OUTPATH)
	

clean:
	@rm -rf $(OBJS)

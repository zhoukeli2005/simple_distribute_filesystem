# inherit CC CFLAGS LFLAGS AR NETLIB LIBPATH from parent Makefile

OUTPATH = ../$(LIBPATH)

CFLAGS += -I../include/

OBJS = s_queue.o s_string.o s_array.o s_hash.o s_config.o s_num_str.o

.PHONY: all clean

all: $(OBJS)
	@mv $(OBJS) $(OUTPATH)
	

clean:
	@rm -rf $(OBJS)

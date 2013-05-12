# inherit CC CFLAGS LFLAGS AR NETLIB LIBPATH from parent Makefile

OUTPATH = ../$(LIBPATH)

CFLAGS += -I../include/

OBJS = s_queue.o s_string.o s_array.o s_hash.o s_config.o s_num_str.o s_mem.o	\
       s_lock_queue.o s_1r1w_queue.o s_spinlock_queue.o

.PHONY: all clean

all: $(OBJS)
	@cp $(OBJS) $(OUTPATH)
	

clean:
	@rm -rf $(OBJS)

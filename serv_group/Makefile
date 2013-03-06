# inherit CC CFLAGS LFLAGS AR NETLIB LIBPATH from parent Makefile

OUTPATH = ../$(LIBPATH)

CFLAGS += -I../include/

OBJS = s_server_group.o s_servg_check_list.o s_servg_do_event.o

.PHONY: all clean

all: $(OBJS)
	@mv $(OBJS) $(OUTPATH)
	

clean:
	@rm -rf $(OBJS)

# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

CFLAGS += -I../include/

OBJS = s_main.o s_server.o	\
       s_do_net_event_m.o s_do_net_event_d.o s_do_net_event_c.o

LIBS = ../$(LIB)

.PHONY: all clean $(OUT)

all: $(OBJS)
	$(CC) $(LFLAGS) -o $(MSERVER) $(OBJS) $(LIBS)
	cp $(MSERVER) $(MSERVER)_1
	cp $(MSERVER) $(MSERVER)_2
	rm $(MSERVER) -rf


clean:
	@rm -rf $(OBJS)
	@rm -rf $(MSERVER)
	@rm -rf $(MSERVER)_1
	@rm -rf $(MSERVER)_2


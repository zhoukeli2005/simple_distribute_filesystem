# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

CFLAGS += -I../include/

OBJS = s_main.o \
       s_ud_client.o

LIBS = ../$(LIB)

.PHONY: all clean $(OUT)

all: $(OBJS)
	$(CC) $(LFLAGS) -o $(MSERVER) $(OBJS) $(LIBS)
	cp $(MSERVER) $(MSERVER)_1
	cp $(MSERVER) $(MSERVER)_2
	cp $(MSERVER) $(MSERVER)_3
	cp $(MSERVER) $(MSERVER)_4
	cp $(MSERVER) $(MSERVER)_5
	rm $(MSERVER) -rf


clean:
	@rm -rf $(OBJS)
	@rm -rf $(MSERVER)_*


# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

CFLAGS += -I../include/

OBJS = s_main.o s_server.o	\
       s_ud_client.o

LIBS = ../$(LIB)

.PHONY: all clean $(OUT)

all: $(OBJS)
	$(CC) $(LFLAGS) -o $(MSERVER) $(OBJS) $(LIBS)
	cp $(MSERVER) $(MSERVER)_1
	cp $(MSERVER) $(DSERVER)_1
	cp $(MSERVER) $(DSERVER)_2
	cp $(MSERVER) $(DSERVER)_3
	cp $(MSERVER) $(DSERVER)_4
	cp $(MSERVER) $(DSERVER)_5
	cp $(MSERVER) $(CLIENT)_1
	rm $(MSERVER) -rf


clean:
	@rm -rf $(OBJS)
	@rm -rf $(MSERVER)
	@rm -rf $(MSERVER)_1
	@rm -rf $(DSERVER)_1
	@rm -rf $(DSERVER)_2
	@rm -rf $(DSERVER)_3
	@rm -rf $(DSERVER)_4
	@rm -rf $(DSERVER)_5
	@rm -rf $(CLIENT)_1


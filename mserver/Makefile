# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

CFLAGS += -I../include/

OBJS = s_main.o s_server.o

LIBS = ../$(LIB)

.PHONY: all clean $(OUT)

all: $(OBJS)
	$(CC) $(LFLAGS) -o $(MSERVER) $(OBJS) $(LIBS)
	cp $(MSERVER) $(MSERVER)_1
	cp $(MSERVER) $(MSERVER)_2
	cp $(MSERVER) $(DSERVER)_1
	cp $(MSERVER) $(DSERVER)_2
	cp $(MSERVER) $(CLIENT)
	rm $(MSERVER) -rf


clean:
	@rm -rf $(OBJS)
	@rm -rf $(MSERVER)
	@rm -rf $(MSERVER)_1
	@rm -rf $(MSERVER)_2
	@rm -rf $(DSERVER)_1
	@rm -rf $(DSERVER)_2
	@rm -rf $(CLIENT)


# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

CFLAGS += -I../include/

OBJS = s_main.o

LIBS = ../$(LIB)

.PHONY: all clean

all: $(OBJS)
	  $(CC) $(LFLAGS) -o $(CLIENT) $(OBJS) $(LIBS)

clean:
	@rm -rf $(OBJS)
	@rm -rf $(CLIENT)

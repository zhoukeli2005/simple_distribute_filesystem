# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

CFLAGS += -I../include/

OBJS = s_main.o

LIBS = ../$(LIB)

.PHONY: all clean $(OUT)

all: $(OBJS)
	  $(CC) $(LFLAGS) -o $(MSERVER) $(OBJS) $(LIBS)

clean:
	@rm -rf $(OBJS)
	@rm -rf $(MSERVER)

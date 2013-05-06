# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

CFLAGS += -I../include/

OBJS = s_main.o

LIBS = ../$(LIB)

TEST = s_test

.PHONY: all clean $(OUT)

all: $(OBJS)
	$(CC) -o $(TEST) $(OBJS) $(LIBS) $(LFLAGS)


clean:
	@rm -rf $(OBJS)
	@rm -rf $(TEST)


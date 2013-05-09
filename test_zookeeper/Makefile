# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

CFLAGS += -I/usr/include/zookeeper
LFLAGS += -lzookeeper_mt

OBJS = s_main.o

TEST = s_test

.PHONY: all clean $(OUT)

all: $(OBJS)
	$(CC) -o $(TEST) $(OBJS) $(LIBS) $(LFLAGS)


clean:
	@rm -rf $(OBJS)
	@rm -rf $(TEST)


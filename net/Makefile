# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

OUT = $(NETLIB)

CFLAGS += -I../include/

OBJS = s_net.o

.PHONY: all clean $(OUT)

all: $(OUT)

$(OUT): $(OBJS)
	  $(AR) -r $(OUT) $(OBJS)

clean:
	@rm -rf $(OBJS)
	@rm -rf $(OUT)

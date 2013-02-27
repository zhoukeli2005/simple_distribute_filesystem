# inherit CC CFLAGS LFLAGS from parent Makefile

OUT = $(THREADLIB)

CFLAGS += -I../include/

OBJS = s_thread.o

.PHONY: all clean $(OUT)

all: $(OUT)

$(OUT): $(OBJS)
	  $(AR) -r $(OUT) $(OBJS)

clean:
	@rm -rf $(OBJS)
	@rm -rf $(OUT)

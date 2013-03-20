# inherit CC CFLAGS LFLAGS from parent Makefile

OUTPATH = ../$(LIBPATH)

CFLAGS += -I../include/

OBJS = s_thread.o

.PHONY: all clean

all: $(OBJS)
	  @cp $(OBJS) $(OUTPATH)

clean:
	@rm -rf $(OBJS)

# inherit CC CFLAGS LFLAGS AR NETLIB from parent Makefile

OUTPATH = ../$(LIBPATH)

CFLAGS += -I../include/

OBJS = s_file_reader.o

.PHONY: all clean

all: $(OBJS)
	  @cp $(OBJS) $(OUTPATH)

clean:
	@rm -rf $(OBJS)

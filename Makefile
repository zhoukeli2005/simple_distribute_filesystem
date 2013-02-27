CC = gcc
CFLAGS = -g
LFLAGS =
AR = ar

# libs
THREADLIB = thread.a
NETLIB = net.a

.PHONY : all clean

export CC CFLAGS LFLAGS AR
export THREADLIB NETLIB

all:
	make -C ./thread
#	make -C ./net $(MAKE_PARAM)
#	make -C ./client $(MAKE_PARAM)
#	make -C ./mserver $(MAKE_PARAM)
#	make -C ./dserver $(MAKE_PARAM)

clean:
	make -C ./thread clean
#	make -C ./net clean
#	make -C ./client clean
#	make -C ./mserver clean
#	make -C ./dserver clean

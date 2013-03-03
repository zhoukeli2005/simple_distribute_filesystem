CC = gcc
CFLAGS = -g
LFLAGS =
AR = ar

# libs
THREADLIB = thread.a
NETLIB = net.a
FSLIB = fs.a

LIBPATH = lib/

OBJS = $(LIBPATH)*.o
LIB = $(LIBPATH)s_lib.a

# exec
MSERVER = s_mserver
DSERVER = s_dserver
CLIENT = s_client

.PHONY : all clean

export CC CFLAGS LFLAGS AR
export LIBPATH LIB
export MSERVER DSERVER CLIENT

all:
	make -C ./common
	make -C ./thread
	make -C ./net
	make -C ./fs
	ar -r $(LIB) $(OBJS)
	make -C ./mserver
	make -C ./client

clean:
	make -C ./common clean
	make -C ./thread clean
	make -C ./net clean
	make -C ./fs clean
	make -C ./mserver clean
	make -C ./client clean
	@rm -rf $(OBJS)
	@rm -rf $(LIB)


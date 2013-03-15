CC = gcc
CFLAGS = -g -DS_DEBUG -Wall
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
	make -C ./misc
	make -C ./thread
	make -C ./net
	make -C ./fs
	make -C ./serv_group
	make -C ./core
	ar -r $(LIB) $(OBJS)
	make -C ./mserver
#	make -C ./dserver
#	make -C ./client

clean:
	make -C ./common clean
	make -C ./misc clean
	make -C ./thread clean
	make -C ./net clean
	make -C ./fs clean
	make -C ./serv_group clean
	make -C ./core clean
	make -C ./mserver clean
#	make -C ./dserver clean
#	make -C ./client clean
	@rm -rf $(OBJS)
	@rm -rf $(LIB)


#include <s_net.h>
#include <s_packet.h>
#include <s_common.h>

#include <socket.h>

struct s_conn {
	struct s_net * net;
	struct s_queue * write_queue;

	// read buffer
	char * read_buf;
	int read_buf_p;
	int read_buf_sz;
};

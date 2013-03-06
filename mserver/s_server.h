#ifndef s_server_h_
#define s_server_h_

#include <s_net.h>
#include <s_packet.h>
#include <s_list.h>
#include <s_array.h>
#include <s_config.h>
#include <s_common.h>
#include <s_thread.h>
#include <s_ipc.h>
#include <s_server_group.h>

struct s_mserver;

struct s_mserver {
	int id;

	struct s_server_group * servg;
};

struct s_mserver * 
s_mserver_create(int argc, char * argv[], struct s_config * config);

#endif


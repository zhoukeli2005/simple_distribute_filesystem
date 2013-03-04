#ifndef s_server_h_
#define s_server_h_

#include <s_net.h>
#include <s_packet.h>
#include <s_list.h>
#include <s_array.h>
#include <s_config.h>
#include <s_common.h>
#include <s_thread.h>

#include <sys/time.h>

#define S_SERV_TYPE_M	1
#define S_SERV_TYPE_D	2

// if server is connected and established
#define S_SERV_FLAG_ESTABLISH	(1 << 0)
// if server is waiting for pong
#define S_SERV_FLAG_SEND_PING	(1 << 1)
// if server is a valid server
#define S_SERV_FLAG_VALID	(1 << 2)

struct s_server {
	struct s_conn * conn;

	// type -- is meta-server or data-server
	int type;

	// flags
	unsigned int flags;

	// infomation
	struct s_string * ip;
	int port;

	// time
	struct timeval tv_established;
	struct timeval tv_send_ping;
	struct timeval tv_receive_pong;

	struct timeval tv_delay;

	// heart beat
	struct s_list wait_for_ping_list;
	struct s_list wait_for_pong_list;
};

struct s_mserver {
	int id;

	// config
	struct s_config * config;

	// net
	struct s_net * net;
	struct s_thread * net_thread;

	// mservs and dservs
	struct s_array * mservs;
	struct s_array * dservs;

	// list for ping/pong
	struct s_list wait_for_ping_list;
	struct s_list wait_for_pong_list;
};

struct s_mserver * 
s_mserver_create(int argc, char * argv[]);

int
s_mserver_init_config(struct s_mserver * mserv, struct s_config * config);

#endif


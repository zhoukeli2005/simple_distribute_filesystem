#ifndef s_servg_h_
#define s_servg_h_

#include <s_server_group.h>
#include <s_net.h>
#include <s_packet.h>
#include <s_list.h>
#include <s_array.h>
#include <s_string.h>
#include <s_config.h>
#include <s_common.h>
#include <s_thread.h>
#include <s_ipc.h>

/* ------ Serv Flags ------------ */
#define S_SERV_FLAG_ESTABLISHED (1 << 0)        // server is connected and established
#define S_SERV_FLAG_IN_CONFIG   (1 << 1)        // server is in config.conf

struct s_server {
	struct s_server_group * servg;

	struct s_conn * conn;

	// id and type
	int id; 
	int type;

	// flags
	unsigned int flags;

	// infomation
	struct s_string * ip; 
	int port;

	// time
	struct timeval tv_connect;
	struct timeval tv_send_identify;
	struct timeval tv_established;
	struct timeval tv_send_ping;
	struct timeval tv_receive_pong;

	struct timeval tv_delay;

	// server can be : wait_for_conn, wait_for_identify, wait_for_ping, wait_for_pong
	struct s_list list;

	// user data
	void * udata;
};

struct s_server_group {
	// this server's id and type
	int id;
	int type;

	// config -- {
		struct s_config * config;

		// password between ipc
		unsigned int ipc_pwd;

		// sec for timeover : default 5
		int sec_for_timeover;
		int sec_for_pingpong;
		int sec_for_reconnect;

	// -- }

	// net
	struct s_net * net;

	// mservs and dservs and clients
	struct s_array * servs[S_SERV_TYPE_MAX];

	struct s_server * min_delay_serv[S_SERV_TYPE_MAX];

	// list for ping/pong
	struct s_list list_wait_for_ping;
	struct s_list list_wait_for_pong;

	// list for connect
	struct s_list list_wait_for_conn;
	struct s_list list_wait_for_identify;

	// callback
	struct s_servg_callback callback;
};

struct s_server *
s_servg_create_serv(struct s_server_group * servg);

void
s_servg_reset_serv(struct s_server_group * servg, struct s_server * serv);

int
s_servg_check_list(struct s_server_group * servg);

void
s_servg_do_event(struct s_conn * conn, struct s_packet * pkt, void * ud);

#endif


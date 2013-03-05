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

#include <sys/time.h>

struct s_server;
struct s_client;
struct s_mserver;

/* ------ Serv Types ----------- */
#define S_SERV_TYPE_M	1	// meta server
#define S_SERV_TYPE_D	2	// data server
#define S_SERV_TYPE_C	3	// client server

/* ------ Serv Flags ------------ */
#define S_SERV_FLAG_ESTABLISH	(1 << 0)	// server is connected and established
#define S_SERV_FLAG_SEND_PING	(1 << 1)	// server is waiting for pong
#define S_SERV_FLAG_VALID	(1 << 2)	// server is a valid server
#define S_SERV_FLAG_IDENTIFY	(1 << 3)	// server is waiting for identification answer

#define S_SERV_CLIENT_HEADER	\
	struct s_conn * conn;	\
	struct s_mserver * mserv;	\
	int type

struct s_server {
	S_SERV_CLIENT_HEADER;

	int id;

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
};

/* --- Client Flags --- */
#define S_CLIENT_FLAG_ESTABLISHED	(1 << 0)	// client is connected and authed

struct s_client {
	S_SERV_CLIENT_HEADER;

	int flags;

	// timeval
	struct timeval tv_connect;
	struct timeval tv_established;

	// all request packets
	struct s_list req_list;

	// current list
	struct s_list list;

	// statistic
	int nreq;
};

struct s_serv_client_header {
	S_SERV_CLIENT_HEADER;
};

struct s_mserver {
	int id;

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
	struct s_thread * net_thread;

	// mservs and dservs
	struct s_array * mservs;
	struct s_array * dservs;

	// list for ping/pong
	struct s_list wait_for_ping_list;
	struct s_list wait_for_pong_list;

	// list for connect
	struct s_list wait_for_conn_list;
	struct s_list wait_for_identify_list;

	// list for no-authed client
	struct s_list client_no_auth_list;
};

struct s_mserver * 
s_mserver_create(int argc, char * argv[]);

int
s_mserver_init_config(struct s_mserver * mserv, struct s_config * config);

struct s_server *
s_mserver_get_serv(struct s_mserver * mserv, int type, int id);

/*
 *	s_mserver_clear_server
 *
 *	@explan: when a server has error (connection broken, invalid packet, etc...)
 *		 clear the server and init a new one (wait for connection or connect to)
 */
void
s_mserver_clear_server(struct s_mserver * mserv, struct s_server * serv);

struct s_client *
s_mserver_create_client(struct s_mserver * mserv, struct s_conn * conn);

void
s_mserver_destroy_client(struct s_mserver * mserv, struct s_client * client);

#endif


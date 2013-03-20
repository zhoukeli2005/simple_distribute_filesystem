#ifndef s_server_group_h_
#define s_server_group_h_

#include <s_net.h>
#include <s_packet.h>
#include <s_config.h>
#include <s_common.h>
#include <s_ipc.h>

struct s_server;
struct s_server_group;

/* ------ Serv Types ----------- */
#define S_SERV_TYPE_C	1	// client server
#define S_SERV_TYPE_D	2	// data server
#define S_SERV_TYPE_M	3	// meta server

#define S_SERV_TYPE_MAX	4

typedef void(* S_SERVG_CALLBACK)(struct s_server * serv, struct s_packet * pkt, void * ud);

struct s_servg_callback {
	void * udata;

	void(*serv_established)	(struct s_server * serv, void * udata);
	void(*serv_closed)	(struct s_server * serv, void * udata);

	void(*all_established) (void * udata);


	S_SERVG_CALLBACK handle_msg[S_SERV_TYPE_MAX][S_PKT_TYPE_MAX_];
};


/*
 *	s_servg_create
 *
 *	@param type : this server's type
 *	@param id : this server's id
 *	@param callback :
 */
struct s_server_group * 
s_servg_create(int type, int id, struct s_servg_callback * callback);


/*
 *	s_servg_register_
 *
 *	@param serv_type : server's type
 *	@param fun : fun
 *	@param callback :
 */
int
s_servg_register(struct s_server_group * servg, int serv_type, int fun, S_SERVG_CALLBACK callback);

/*
 *	s_servg_init_config
 *
 *	@explain : init server group from config
 */
int
s_servg_init_config(struct s_server_group * servg, struct s_config * config);


/*
 *	s_servg_poll
 *
 *	@explain : let server-group run for one step
 *		   1. maintain all servers' state
 *		   2. ping-pong each server
 *		   3. keep-alive with each server
 *		   4. connect/reconnect each server
 *		   5. receive packet from each server and dispatch
 *		   6. send packet to each server
 */
int
s_servg_poll(struct s_server_group * servg, int msec);


/*
 *	iterator servers & get min delay server
 *
 */
struct s_server *
s_servg_next_active_serv(struct s_server_group * servg, int type, int * id);

struct s_server *
s_servg_get_active_serv(struct s_server_group * servg, int type, int id);

struct s_array *
s_servg_get_serv_array(struct s_server_group * servg, int type);

struct s_server *
s_servg_get_min_delay_serv(struct s_server_group * servg, int type);


/*
 *	get conn from server & get server from conn
 *
 */
/*
struct s_conn *
s_servg_get_conn(struct s_server * serv);

struct s_server *
s_servg_get_serv_from_conn(struct s_conn * conn);
*/

/*
 *	get information
 *
 */
int
s_servg_get_id(struct s_server * serv);

int
s_servg_get_type(struct s_server * serv);

unsigned int
s_servg_get_mem(struct s_server * serv);


/*
 *	get user-data from server
 *
 */
void
s_servg_set_udata(struct s_server * serv, void * ud);

void *
s_servg_get_udata(struct s_server * serv);

struct s_server_group *
s_servg_get_g(struct s_server * serv);

void *
s_servg_gdata(struct s_server_group * servg);

#define s_servg_gdata_from_serv(serv)	s_servg_gdata(s_servg_get_g(serv))

/*
 *	rpc
 *
 */
int
s_servg_rpc_call(struct s_server * serv, struct s_packet * pkt, void * ud, S_SERVG_CALLBACK callback, int msec_timeout);

void
s_servg_rpc_ret(struct s_server * serv, unsigned int req_id, struct s_packet * pkt);

#define s_servg_send(serv, pkt) 	\
	s_servg_rpc_call(serv, pkt, NULL, NULL, -1)

#endif


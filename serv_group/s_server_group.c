#include "s_server_group.h"
#include <s_mem.h>
#include <s_hash.h>
#include <s_ipc.h>
#include "s_servg.h"

static struct s_server_group g_serv;

struct s_server_group * s_servg_create(int type, int id, struct s_servg_callback * callback)
{
	struct s_server_group * servg = &g_serv;
	memset(servg, 0, sizeof(struct s_server_group));

	servg->id = id;
	servg->type = type;

	int i;
	for(i = 0; i < S_SERV_TYPE_MAX; ++i) {
		servg->servs[i] = s_array_create(sizeof(struct s_server), 16);
		if(!servg->servs[i]) {
			return NULL;
		}
		servg->min_delay_serv[i] = NULL;
	}

	s_list_init(&servg->list_wait_for_conn);
	s_list_init(&servg->list_wait_for_identify);
	s_list_init(&servg->list_wait_for_ping);
	s_list_init(&servg->list_wait_for_pong);

	memset(&servg->callback, 0, sizeof(struct s_servg_callback));
	if(callback) {
		memcpy(&servg->callback, callback, sizeof(struct s_servg_callback));
	}

	s_list_init(&servg->rpc_timeout_list);

	return servg;
}

int s_servg_register(struct s_server_group * servg, int serv_type, int fun, S_SERVG_CALLBACK callback)
{
	if(serv_type < 0 || serv_type >= S_SERV_TYPE_MAX) {
		s_log("[Error] servg_register invalid serv_type:%d", serv_type);
		return -1;
	}
	if(fun <= S_PKT_TYPE_USER_DEF_BEGIN_ || fun >= S_PKT_TYPE_MAX_) {
		s_log("[Error] servg_register invalid fun:%d", fun);
		return -1;
	}
	if(servg->callback.handle_msg[serv_type][fun]) {
		s_log("[Error] servg_register serv_type:%d, fun:%d. double-register!!", serv_type, fun);
		return -1;
	}
	servg->callback.handle_msg[serv_type][fun] = callback;
	return 0;
}

static int iinit_serv(struct s_array * array, int id);
static int iparse_config_serv_type_id(const char * p, int * type, int * id);

int s_servg_init_config(struct s_server_group * servg, struct s_config * config)
{
	servg->config = config;

	struct s_string * ip_str = s_string_create("ip");
	struct s_string * port_str = s_string_create("port");
	if(!ip_str || !port_str) {
		s_log("[Error] no mem!");
		return -1;
	}

	// default region
	if(s_config_select(servg->config, "default") < 0) {
		s_log("[Error:Config] no default in config!");
		return -1;
	}
	servg->sec_for_timeover = s_config_read_i(servg->config, "sec_for_timeover");
	if(servg->sec_for_timeover <= 0) {
		s_log("[Error:Config] no sec_for_timeover in config! use default:5!");
		return -1;
	}

	s_log("[LOG:Config] sec for timeover:%d", servg->sec_for_timeover);

	servg->sec_for_pingpong = s_config_read_i(servg->config, "sec_for_pingpong");
	if(servg->sec_for_pingpong <= 0) {
		s_log("[Error:Config] no sec_for_pingpong in config!");
		return -1;
	}
	s_log("[LOG:Config] sec for pingpong:%d", servg->sec_for_pingpong);

	servg->sec_for_reconnect = s_config_read_i(servg->config, "sec_for_reconnect");
	if(servg->sec_for_reconnect <= 0) {
		s_log("[Error:Config] no sec_for_reconnect in config!");
		return -1;
	}
	s_log("[LOG:Config] sec for reconnect:%d", servg->sec_for_reconnect);

	servg->ipc_pwd = s_config_read_i(servg->config, "ipc_pwd");
	if(servg->ipc_pwd < 0) {
		s_log("[Error:Config] no ipc pwd in config!");
		return -1;
	}
	s_log("[LOG:Config] ipc pwd:%d", servg->ipc_pwd);

	// init all serv
	s_config_iter_begin(servg->config);
	struct s_string * next = s_config_iter_next(servg->config);
	while(next) {
		struct s_string * region = next;
		next = s_config_iter_next(servg->config);
		const char * p = s_string_data_p(region);
		int type, id;
		if(iparse_config_serv_type_id(p, &type, &id) < 0) {
			s_log("[LOG:Config]not serv. region:%s", p);
			continue;
		}

		if(s_config_select_region(servg->config, region) < 0) {
			s_log("[Error:Config] select region(%s) error!", p);
			return -1;
		}

		struct s_string * ip = s_config_read_string(servg->config, ip_str);
		int port = s_config_read_int(servg->config, port_str);

		s_log("[LOG] select region:%s, ip:%p, port:%d", p, s_string_data_p(ip), port);

		struct s_array * servs = servg->servs[type];
		if(iinit_serv(servs, id) < 0) {
			s_log("[Error] init serv (%d) error!", id);
			return -1;
		}

		struct s_server * serv = s_array_at(servs, id);
		serv->servg = servg;
		serv->conn = NULL;

		serv->type = type;
		serv->id = id;

		serv->req_id = 1;
		serv->rpc_hash = NULL;

		// information
		if(ip) {
			s_string_grab(ip);
		}
		serv->ip = ip;
		serv->port = port;
		serv->mem = 0;

		if(type == servg->type && id == servg->id) {
			// myself
			s_log("[LOG] find myself in config.");
			servg->net = s_net_create(port, &s_servg_do_event, servg);
			if(!servg->net) {
				s_log("[Error] net init error!port:%d", port);
				return -1;
			}
		}

		// timer
		timerclear(&serv->tv_connect);
		timerclear(&serv->tv_send_identify);
		timerclear(&serv->tv_established);
		timerclear(&serv->tv_send_ping);
		timerclear(&serv->tv_receive_pong);
		timerclear(&serv->tv_delay);

		// wait for connect
		s_list_init(&serv->list);
		if(serv->ip && serv->port && s_do_i_conn_him(servg->type, servg->id, type, id)) {
			s_list_insert_tail(&servg->list_wait_for_conn, &serv->list);
		}
	}

	// if not find myself in config
	if(!servg->net) {
		int port = 0;
		if(servg->type == S_SERV_TYPE_M) {
			port = S_SERV_M_DEFAULT_PORT + servg->id;
			s_log("[LOG] mserv init at default port:%d", port);
		}
		servg->net = s_net_create(port, &s_servg_do_event, servg);
		if(!servg->net) {
			s_log("[Error] net create error!");
			return -1;
		}
	}

	s_string_drop(ip_str);
	s_string_drop(port_str);

	return 0;
}

int s_servg_poll(struct s_server_group * servg, int msec)
{
	/* --- check server connection / reconnection / ping-pong --- */
	if(s_servg_check_list(servg) < 0) {
		return -1;
	}

	/* --- check rpc timeout --- */
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	struct s_list * p, *tmp;
	s_list_foreach_safe(p, tmp, &servg->rpc_timeout_list) {
		struct s_servg_rpc_param * param = s_list_entry(p, struct s_servg_rpc_param, timeout_list);
		if(timercmp(&tv_now, &param->tv_timeout, >=)) {
			// timeout
			param->callback(param->serv, NULL, param->ud);

			s_list_del(p);

			s_hash_del_num(param->serv->rpc_hash, param->req_id);
			s_free(param);
		}
	}

	/* --- do net poll --- */
	return s_net_poll(servg->net, msec);
}

static struct s_server * iget_serv(struct s_server_group * servg, int type, int id)
{
	if(type >= 0 && type < S_SERV_TYPE_MAX) {
		struct s_server * serv = s_array_at(servg->servs[type], id);
		return serv;
	}
	s_log("invalid param, type:%d, id:%d", type, id);
	return NULL;
}

struct s_server * s_servg_get_serv_in_config(struct s_server_group * servg, int type, int id)
{
	struct s_server * serv = iget_serv(servg, type, id);
	if(serv && serv->flags & S_SERV_FLAG_IN_CONFIG) {
		return serv;
	}
	return NULL;
}

struct s_server * s_servg_get_active_serv(struct s_server_group * servg, int type, int id)
{
	struct s_server * serv = iget_serv(servg, type, id);
	if(serv && (serv->flags & S_SERV_FLAG_ESTABLISHED)) {
		return serv;
	}
	return NULL;
}

struct s_array * s_servg_get_serv_array(struct s_server_group * servg, int type)
{
	if(type >= 0 && type < S_SERV_TYPE_MAX) {
		return servg->servs[type];
	}
	s_log("[Error] get_serv_array:invalid param, type:%d", type);
	return NULL;
}

struct s_server * s_servg_get_min_delay_serv(struct s_server_group * servg, int type)
{
	struct s_server * serv = servg->min_delay_serv[type];
	if(serv) {
		return serv;
	}
	int i;
	struct s_array * array = servg->servs[type];
	for(i = 0; i < s_array_len(array); ++i) {
		serv = s_array_at(array, i);
		if((serv->flags & S_SERV_FLAG_IN_CONFIG) && (serv->flags & S_SERV_FLAG_ESTABLISHED)) {
			return serv;
		}
	}
	return NULL;
}

int s_servg_get_id(struct s_server * serv)
{
	return serv->id;
}

int s_servg_get_type(struct s_server * serv)
{
	return serv->type;
}

unsigned int s_servg_get_mem(struct s_server * serv)
{
	return serv->mem;
}

/*
struct s_conn * s_servg_get_conn(struct s_server * serv)
{
	return serv->conn;
}

struct s_server * s_servg_get_serv_from_conn(struct s_conn * conn)
{
	return (struct s_server *)s_net_get_udata(conn);
}*/

void s_servg_set_udata(struct s_server * serv, void * ud)
{
	serv->udata = ud;
}

void * s_servg_get_udata(struct s_server * serv)
{
	return serv->udata;
}

struct s_server * s_servg_next_active_serv(struct s_server_group * servg, int type, int * id)
{
	int i = s_max(0, *id);
	struct s_array * array = s_servg_get_serv_array(servg, type);
	if(!array) {
		return NULL;
	}
	int len = s_array_len(array);
	for(; i < len; ++i) {
		struct s_server * serv = s_array_at(array, i);
		if(serv->flags & S_SERV_FLAG_IN_CONFIG && serv->flags & S_SERV_FLAG_ESTABLISHED) {
			*id = i + 1;
			return serv;
		}
	}

	return NULL;
}

struct s_server_group * s_servg_get_g(struct s_server * serv)
{
	return serv->servg;
}

void * s_servg_gdata(struct s_server_group * servg)
{
	return servg->callback.udata;
}

int s_servg_rpc_call(struct s_server * serv, struct s_packet * pkt, void * ud, S_SERVG_CALLBACK callback, int msec_timeout)
{
	if(!callback) {
		// normal send
		s_net_send(serv->conn, pkt);
		return 0;
	}
	struct s_server_group * servg = serv->servg;

	unsigned int req_id = serv->req_id++;
	if(s_packet_set_req(pkt, req_id) < 0) {
		s_log("[Warning] rpc_call : write req(%u) failed!", req_id);
		return -1;
	}

	struct s_servg_rpc_param * param = s_malloc(struct s_servg_rpc_param, 1);
	if(!param) {
		s_log("[Error] no mem for rpc_param!");
		return -1;
	}
	param->req_id = req_id;
	param->ud = ud;
	param->callback = callback;
	param->msec_timeout = msec_timeout;
	param->serv = serv;

	s_list_init(&param->timeout_list);

	// add timeout
	if(msec_timeout > 0) {
		gettimeofday(&param->tv_timeout, NULL);
		param->tv_timeout.tv_usec += msec_timeout * 1000;
		while(param->tv_timeout.tv_usec > 1000000) {
			param->tv_timeout.tv_sec++;
			param->tv_timeout.tv_usec -= 1000000;
		}

		struct s_list * p;
		s_list_foreach(p, &servg->rpc_timeout_list) {
			struct s_servg_rpc_param * next_param = s_list_entry(p, struct s_servg_rpc_param, timeout_list);
			if(timercmp(&next_param->tv_timeout, &param->tv_timeout, >=)) {
				break;
			}
		}
		s_list_insert_tail(p, &param->timeout_list);
	}

	// add to rpc_hash
	struct s_servg_rpc_param ** pp = s_hash_set_num(serv->rpc_hash, req_id);
	*pp = param;

	// send packet
	s_net_send(serv->conn, pkt);
	return 0;
}

void s_servg_rpc_ret(struct s_server * serv, unsigned int req_id, struct s_packet * pkt)
{
	s_packet_set_fun(pkt, S_PKT_TYPE_RPC_BACK_);
	s_packet_set_req(pkt, req_id);

	s_net_send(serv->conn, pkt);
}

struct s_server * s_servg_connect(struct s_server_group * servg, int type, int id, const char * ip, int port)
{
	struct s_server * serv = s_servg_get_serv_in_config(servg, type, id);
	if(serv) {
		s_log("[Warning] s_servg_connect(type:%d, id:%d), already in IN_CONFIG", type, id);
		return NULL;
	}
	serv = s_servg_get_active_serv(servg, type, id);
	if(serv) {
		s_log("[Warning] s_servg_connect(type:%d, id:%d), already active!", type, id);
		return serv;
	}
	serv = s_servg_create_serv(servg, type, id);
	if(!serv) {
		s_log("[Warning] s_servg_connect(type:%d, id:%d), create_serv failed!", type, id);
		return NULL;
	}

	gettimeofday(&serv->tv_connect, NULL);
	serv->conn = s_net_connect(servg->net, ip, port);
	if(!serv) {
		s_log("[Warning] s_servg_connect(type:%d, id:%d), net_connect(%s:%d) failed!", type, id, ip, port);
		s_servg_reset_serv(servg, serv);
		return NULL;
	}

	s_net_set_udata(serv->conn, serv);

	s_list_insert_tail(&servg->list_wait_for_identify, &serv->list);

	serv->tv_send_identify = serv->tv_connect;

	// send identification
	struct s_packet * pkt = s_servg_pkt_identify(servg->type, servg->id, servg->ipc_pwd);
	s_net_send(serv->conn, pkt);
	s_packet_drop(pkt);

	return serv;
}

static S_LIST_DECLARE(g_free_serv);

struct s_server * s_servg_create_serv(struct s_server_group * servg, int type, int id)
{
	struct s_array * array = s_servg_get_serv_array(servg, type);
	if(!array) {
		return NULL;
	}

	if(id > 0) {
		iinit_serv(array, id);
		struct s_server * serv = s_array_at(array, id);
		serv->flags = 0;
		serv->id = id;
		serv->type = type;
		serv->servg = servg;
		s_list_init(&serv->list);
		return serv;
	}

	int i;
	int len = s_array_len(array);
	struct s_server * serv = NULL;
	for(i = 0; i < len; ++i) {
		struct s_server * serv = s_array_at(array, i);
		if(!(serv->flags & S_SERV_FLAG_IN_CONFIG) && !(serv->flags & S_SERV_FLAG_ESTABLISHED)) {
			id = i;
			break;
		}
	}
	if(!serv) {
		id = len;
		serv = s_array_push(array);
	}
	memset(serv, 0, sizeof(struct s_server));
	serv->id = id;
	serv->type = type;
	serv->servg = servg;
	s_list_init(&serv->list);
	return serv;

	/*
	struct s_server * serv;
	if(!s_list_empty(&g_free_serv)) {
		struct s_list * p = s_list_first(&g_free_serv);
		s_list_del(p);
		serv = s_list_entry(p, struct s_server, list);
	} else {
		serv = s_malloc(struct s_server, 1);
	}

	memset(serv, 0, sizeof(struct s_server));

	s_list_init(&serv->list);

	serv->servg = servg;

	return serv;
	*/
}

void s_servg_reset_serv(struct s_server_group * servg, struct s_server * serv)
{
	if(serv->conn) {
		s_net_close(serv->conn);
		serv->conn = NULL;
	}

	s_list_del(&serv->list);

	if(servg->min_delay_serv[serv->type] == serv) {
		// it is the low delay server, reset
		servg->min_delay_serv[serv->type] = NULL;
	}

	if((serv->flags & S_SERV_FLAG_IN_CONFIG) == 0) {
		// not in config.conf, just free
		serv->flags = 0;
		return;
	}

	serv->flags = S_SERV_FLAG_IN_CONFIG;

	if(serv->ip && serv->port > 0 && s_do_i_conn_him(servg->type, servg->id, serv->type, serv->id)) {
		s_list_insert_tail(&servg->list_wait_for_conn, &serv->list);
	}
}

static int iinit_serv(struct s_array * array, int id)
{
	int len = s_array_len(array);
	if(id < len) {
		struct s_server * serv = s_array_at(array, id);
		serv->flags = S_SERV_FLAG_IN_CONFIG;
		return 0;
	}

	for(; len < id; ++len) {
		struct s_server * serv = s_array_push(array);
		if(!serv) {
			return -1;
		}
		serv->flags = 0;
		serv->conn = NULL;
	}
	struct s_server * serv = s_array_push(array);
	serv->flags = S_SERV_FLAG_IN_CONFIG;
	return 0;
}

static int iparse_config_serv_type_id(const char * p, int * type, int * id)
{
	*id = 0;
	int len = strlen(p);
	if(len > 8 && strncmp(p, "mserver", 7) == 0) {
		// mserver
		*type = S_SERV_TYPE_M;
		*id = atoi(&p[8]);
	}

	if(len > 8 && strncmp(p, "dserver", 7) == 0) {
		// dserver
		*type = S_SERV_TYPE_D;
		*id = atoi(&p[8]);
	}

	if(len > 7 && strncmp(p, "client", 6) == 0) {
		// client
		*type = S_SERV_TYPE_C;
		*id = atoi(&p[7]);
	}
	if(*id > 0) {
		return 0;
	}
	return -1;
}


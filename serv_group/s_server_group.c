#include <s_server_group.h>
#include <s_mem.h>
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

	return servg;
}

int s_servg_register(struct s_server_group * servg, int serv_type, int fun, S_Servg_Callback_t callback)
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

		// information
		if(ip) {
			s_string_grab(ip);
		}
		serv->ip = ip;
		serv->port = port;

		if(type == servg->type && id == servg->id) {
			// myself
			s_log("[LOG] find myself in config.");
			servg->net = s_net_create(port, &s_servg_do_event, servg);
			if(!servg->net) {
				s_log("[Error] net init error!");
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
		if(type >= servg->type && id > servg->id && serv->ip && serv->port > 0) {
			s_list_insert_tail(&servg->list_wait_for_conn, &serv->list);
		}
	}

	// if not find myself in config
	if(!servg->net) {
		servg->net = s_net_create(0, &s_servg_do_event, servg);
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
	if(s_servg_check_list(servg) < 0) {
		return -1;
	}
	return s_net_poll(servg->net, msec);
}

struct s_server * s_servg_get_serv(struct s_server_group * servg, int type, int id)
{
	if(type >= 0 && type < S_SERV_TYPE_MAX) {
		struct s_server * serv = s_array_at(servg->servs[type], id);
		if(serv && serv->flags & S_SERV_FLAG_IN_CONFIG) {
			return serv;
		}
	}
	s_log("invalid param, type:%d, id:%d", type, id);
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
		if(serv) {
			return serv;
		}
	}
	return NULL;
}

struct s_conn * s_servg_get_conn(struct s_server * serv)
{
	return serv->conn;
}

void s_servg_set_udata(struct s_server * serv, void * ud)
{
	serv->udata = ud;
}

void * s_servg_get_udata(struct s_server * serv)
{
	return serv->udata;
}

static S_LIST_DECLARE(g_free_serv);

struct s_server * s_servg_create_serv(struct s_server_group * servg)
{
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
		s_list_insert(&g_free_serv, &serv->list);
		return;
	}

	serv->flags = S_SERV_FLAG_IN_CONFIG;

	if(serv->type >= servg->type && serv->id > servg->id && serv->ip && serv->port > 0) {
		s_list_insert_tail(&servg->list_wait_for_conn, &serv->list);
	}
}

static int iinit_serv(struct s_array * array, int id)
{
	int len = s_array_len(array);
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


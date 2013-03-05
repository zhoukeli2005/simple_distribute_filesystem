#include "s_server.h"
#include "s_do_net_event.h"

struct s_mserver g_serv;

static int iget_serv_id(const char * p, char sep)
{
	char * p_ = strrchr(p, sep);
	if(!p_) {
		s_log("miss id!");
		return -1;
	}

	int id = atoi(p_ + 1);
	return id;
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
	}
	struct s_server * serv = s_array_push(array);
	serv->flags = S_SERV_FLAG_VALID;
	return 0;
}

struct s_mserver * s_mserver_create(int argc, char * argv[])
{
	struct s_mserver * mserv = &g_serv;

	char * p = argv[0];

	// the exec name is : s_mserver_XX , XX is the id

	int id = iget_serv_id(p, '_');

	if(id <= 0) {
		s_log("id parse error:%s, id:%d", p, id);
		return NULL;
	}

	mserv->id = id;
	mserv->mservs = s_array_create(sizeof(struct s_server), 16);
	mserv->dservs = s_array_create(sizeof(struct s_server), 16);
	if(!mserv->mservs || !mserv->dservs) {
		if(mserv->mservs) {
			s_array_drop(mserv->mservs);
		}
		if(mserv->dservs) {
			s_array_drop(mserv->dservs);
		}
		return NULL;
	}

	s_list_init(&mserv->wait_for_conn_list);
	s_list_init(&mserv->wait_for_identify_list);
	s_list_init(&mserv->wait_for_ping_list);
	s_list_init(&mserv->wait_for_pong_list);

	return mserv;
}

int s_mserver_init_config(struct s_mserver * mserv, struct s_config * config)
{
	struct s_string * id_str = s_string_create_format("mserver-%d", mserv->id);
	if(!id_str) {
		s_log("string format error!");
		return -1;
	}

	mserv->config = config;

	if(s_config_select_region(mserv->config, id_str) < 0) {
		s_log("no region in config!");
		return -1;
	}   

	struct s_string * ip_str = s_string_create("ip");
	if(!ip_str) {
		return -1;
	}

	struct s_string * ip = s_config_read_string(mserv->config, ip_str);
	if(!ip) {
		s_log("no ip!");
		return -1;
	}   
	s_log("ip:%s", s_string_data_p(ip));

	struct s_string * port_str = s_string_create("port");
	if(!port_str) {
		return -1;
	}

	int port = s_config_read_int(mserv->config, port_str);
	if(port <= 0) {
		s_log("no port!");
		return -1;
	}   
	s_log("port:%d", port);

	mserv->net = s_net_create(port, &s_do_net_event, mserv);
	if(!mserv->net) {
		s_log("net create error!");
		return -1;
	}

	// default
	if(s_config_select(mserv->config, "default") < 0) {
		s_log("no default in config!");
		return -1;
	}
	mserv->sec_for_timeover = s_config_read_i(mserv->config, "sec_for_timeover");
	if(mserv->sec_for_timeover <= 0) {
		s_log("no sec_for_timeover in config! use default:5!");
		return -1;
	}

	s_log("sec for timeover:%d", mserv->sec_for_timeover);

	mserv->sec_for_pingpong = s_config_read_i(mserv->config, "sec_for_pingpong");
	if(mserv->sec_for_pingpong <= 0) {
		s_log("no sec_for_pingpong in config!");
		return -1;
	}
	s_log("sec for pingpong:%d", mserv->sec_for_pingpong);

	mserv->sec_for_reconnect = s_config_read_i(mserv->config, "sec_for_reconnect");
	if(mserv->sec_for_reconnect <= 0) {
		s_log("no sec_for_reconnect in config!");
		return -1;
	}
	s_log("sec for reconnect:%d", mserv->sec_for_reconnect);

	// ipc 
	if(s_config_select(mserv->config, "ipc") < 0) {
		s_log("no ipc in config!");
		return -1;
	}
	mserv->ipc_pwd = s_config_read_i(mserv->config, "pwd");
	if(mserv->ipc_pwd < 0) {
		s_log("no ipc pwd in config!");
		return -1;
	}
	s_log("ipc pwd:%d", mserv->ipc_pwd);

	// init all serv
	s_config_iter_begin(mserv->config);
	struct s_string * next = s_config_iter_next(mserv->config);
	while(next) {
		struct s_string * region = next;
		next = s_config_iter_next(mserv->config);
		const char * p = s_string_data_p(region);
		int id = iget_serv_id(p, '-');
		if(id <= 0) {
			s_log("invalid id! serv name:%s", s_string_data_p(region));
			continue;
		}

		if(p[0] != 'm' && p[0] != 'd') {
			continue;
		}

		if(s_config_select_region(mserv->config, region) < 0) {
			s_log("select region error!");
			return -1;
		}

		ip = s_config_read_string(mserv->config, ip_str);
		if(!ip) {
			s_log("no ip for serv:%s", p);
			return -1;
		}

		port = s_config_read_int(mserv->config, port_str);
		if(port <= 0) {
			s_log("no port for serv:%s", p);
			return -1;
		}

		struct s_array * servs = mserv->mservs;
		int type = S_SERV_TYPE_M;

		if(p[0] == 'd') {
			servs = mserv->dservs;
			type = S_SERV_TYPE_D;
		}
		
		if(iinit_serv(servs, id) < 0) {
			s_log("init serv (%d) error!", id);
			return -1;
		}

		struct s_server * serv = s_array_at(servs, id);
		serv->mserv = mserv;
		serv->type = type;
		serv->id = id;

		serv->conn = NULL;
		
		// information
		s_string_grab(ip);
		serv->ip = ip;
		serv->port = port;

		// timer
		timerclear(&serv->tv_connect);
		timerclear(&serv->tv_send_identify);
		timerclear(&serv->tv_established);
		timerclear(&serv->tv_send_ping);
		timerclear(&serv->tv_receive_pong);
		timerclear(&serv->tv_delay);

		// wait for connect
		s_list_init(&serv->list);
		if(type == S_SERV_TYPE_M && id > mserv->id) {
			s_list_insert_tail(&mserv->wait_for_conn_list, &serv->list);
		}
	}

	return 0;
}

struct s_server * s_mserver_get_serv(struct s_mserver * mserv, int type, int id)
{
	if(type == S_SERV_TYPE_M) {
		return s_array_at(mserv->mservs, id);
	}
	if(type == S_SERV_TYPE_D) {
		return s_array_at(mserv->dservs, id);
	}

	s_log("invalid type:%d", type);

	return NULL;
}

void s_mserver_clear_server(struct s_mserver * mserv, struct s_server * serv)
{
	if(serv->conn) {
		s_net_close(serv->conn);
		serv->conn = NULL;
	}

	serv->flags = S_SERV_FLAG_VALID;

	s_list_del(&serv->list);

	if(serv->type == S_SERV_TYPE_M && serv->id > mserv->id) {
		s_list_insert_tail(&mserv->wait_for_conn_list, &serv->list);
	}
}


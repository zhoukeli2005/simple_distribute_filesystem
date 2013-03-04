#include "s_server.h"
#include "s_do_net_event.h"

struct s_mserver g_serv;

static int iget_serv_id(const char * p)
{
	char * p_ = strrchr(p, '_');
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
	while(len < id) {
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

	int id = iget_serv_id(p);

	if(id <= 0) {
		s_log("id parse error:%s, id:%d", p, id);
		return NULL;
	}

	mserv->id = id;
	mserv->mservs = s_array_create(sizeof(struct s_server), 16);
	mserv->dservs = s_array_create(sizeof(struct s_server), 16);
	if(!mserv->mservs || !mserv->dservs) {
		if(mserv->mservs) {
			s_array_put(mserv->mservs);
		}
		if(mserv->dservs) {
			s_array_put(mserv->dservs);
		}
		return NULL;
	}

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
	s_log("ip:%s", s_string_data_p(ip_str));

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

	mserv->net = s_net_create(port, &s_do_net_event);
	if(!mserv->net) {
		s_log("net create error!");
		return -1;
	}

	// init all serv
	s_config_iter_begin(mserv->config);
	struct s_string * region = s_config_iter_next(mserv->config);
	while(region) {
		char * p = s_string_data_p(region);
		int id = iget_serv_id(p);
		if(id <= 0) {
			s_log("invalid id! serv name:%s", s_string_data_p(region));
			return -1;
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

		port = s_config_read_string(mserv->config, port_str);
		if(port <= 0) {
			s_log("no port for serv:%s", p);
			return -1;
		}

		struct s_array * servs = region->mservs;
		int type = S_SERV_TYPE_M;

		if(p[0] == 'd') {
			servs = region->dservs;
			type = S_SERV_TYPE_D;
		}
		
		if(!iinit_serv(servs, id)) {
			s_log("init serv (%d) error!");
			return -1;
		}

		struct s_server * serv = s_array_at(servs, id);
		serv->type = type;
		serv->conn = NULL;
		
		// information
		s_string_get(ip);
		serv->ip = ip;
		serv->port = port;

		// timer
		timerclear(&serv->tv_established);
		timerclear(&serv->tv_send_ping);
		timerclear(&serv->tv_receive_pong);
		timerclear(&serv->tv_delay);

		// ping/pong
		s_list_init(&serv->wait_for_ping_list);
		s_list_init(&serv->wait_for_pong_list);
	}

	return 0;
}


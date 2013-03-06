#include "s_server.h"
#include "s_do_net_event.h"

static struct s_mserver g_serv;

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

struct s_mserver * s_mserver_create(int argc, char * argv[], struct s_config * config)
{
	struct s_mserver * mserv = &g_serv;

	char * p = argv[0];

	// the exec name is : s_mserver_XX , XX is the id

	int id = iget_serv_id(p, '_');

	s_log("[LOG] id:%d", id);

	if(id <= 0) {
		s_log("[Error] id parse error:%s, id:%d", p, id);
		return NULL;
	}

	mserv->id = id;

	struct s_servg_callback callback = {
		.udata = mserv,
		.serv_established = NULL,
		.serv_closed = NULL,
	};
	callback.handle_msg[S_SERV_TYPE_M] = s_do_net_event_m;
	callback.handle_msg[S_SERV_TYPE_D] = s_do_net_event_d;
	callback.handle_msg[S_SERV_TYPE_C] = s_do_net_event_c;

	mserv->servg = s_servg_create(S_SERV_TYPE_M, id, &callback);
	if(!mserv->servg) {
		s_log("[Error] servg create failed!");
		return NULL;
	}

	if(s_servg_init_config(mserv->servg, config) < 0) {
		s_log("[Error] init config error!");
		return NULL;
	}

	return mserv;
}


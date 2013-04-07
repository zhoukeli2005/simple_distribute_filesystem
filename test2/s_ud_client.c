#include "s_server.h"

static void lock_callback(struct s_server * serv, struct s_packet * pkt, void * udata);
static void write_callback(struct s_server * serv, struct s_packet * pkt, void * udata);

struct s_glock_client {
	struct s_core * core;

	struct s_id id;
	unsigned int lock;

	struct timeval tv;
};

static int MaxTry = 0;

static struct timeval gtv;
static int gid;
static int ginit = 0;
static int gcount = 0;

static struct timeval g_tv_start;

static int irand_serv(int * out)
{
	static int k = 0;
	static int serv[][5] = { 
		{1, 2, 3}, 
		{1, 3, 5}, 
		{2, 3, 4}, 
		{3, 4, 5}, 
		{1, 4, 5}, 
		{1, 2, 5}, 
		{0} 
	};  

	int i = k++;
		
	k %= 6;

	memcpy(out, serv[i], sizeof(int) * 5); 

	return 3;
}


void * s_ud_client_init(struct s_core * core)
{
	gettimeofday(&gtv, NULL);
	gid = 0;
	ginit = 1;

	struct s_config * config = core->create_param.config;
	if(s_config_select(config, "default") < 0) {
		s_log("select config.defalut error!");
		exit(0);
	}

	MaxTry = s_config_read_i(config, "MaxTry");

	if(MaxTry <= 0) {
		s_log("select config.default.MaxTry error!");
		exit(0);
	}

	return NULL;
}

void s_ud_client_update(struct s_core * core, void * ud)
{
	if(!ginit) {
		return;
	}

	if(gcount >= MaxTry) {
		return;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct timeval ret;
	timersub(&tv, &gtv, &ret);

	if(ret.tv_usec < 10) {
		return;
	}

	gtv = tv;

	struct s_packet * pkt;
//	struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_PUSH_DATA, 3 * 1024 * 1024);
	struct s_glock_client * c = s_malloc(struct s_glock_client, 1);

	c->id.x = core->id;
	c->id.y = gid++;
	c->tv = tv;
	c->lock = 0;
	c->core = core;


//	s_packet_write_int(pkt, c->id.x);
//	s_packet_write_int(pkt, c->id.y);

	int servs[32];
	int nserv = 0;

	nserv = irand_serv(servs);

	int i;
	for(i = 0; i < 5; ++i) {
		int serv_id = servs[i];
/*		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_D, serv_id);
		if(!serv) {
			s_log("[LOG] missing dserv:%d", serv_id);
			continue;
		}
		s_servg_send(serv, pkt);*/
		if(serv_id > 0) {
			c->lock |= 1 << serv_id;
		}
	}
//	s_packet_drop(pkt);
	//s_log("start to write id(%d,%d) lock:%x ...", c->id.x, c->id.y, c->lock);

	struct s_server * mserv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, 1);
	if(!mserv) {
		s_log("[LOG] missing mserv!");
		return;
	}

	int sz = s_packet_size_for_id(0) +
		s_packet_size_for_uint(0);

	pkt = s_packet_easy(S_PKT_TYPE_GLOBAL_LOCK, sz);

	s_packet_write_int(pkt, c->id.x);
	s_packet_write_int(pkt, c->id.y);
	s_packet_write_uint(pkt, c->lock);

	s_servg_rpc_call(mserv, pkt, c, &lock_callback, -1);

	s_packet_drop(pkt);

	gcount++;
	if(gcount == 1) {
		g_tv_start = tv;
	}
}

static void lock_callback(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_glock_client * c = (struct s_glock_client *)ud;
	struct s_core * core = c->core;

//	s_log("lock free now:%d.%d", c->id.x, c->id.y);

	int sz = s_packet_size_for_id(0);
	pkt = s_packet_easy(S_PKT_TYPE_WRITE, sz);

	s_packet_write_int(pkt, c->id.x);
	s_packet_write_int(pkt, c->id.y);

	int i;
	for(i = 1; i <= 5; ++i) {
		if(!(c->lock & (1 << i))) {
			continue;
		}
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_D, i);
		if(!serv) {
		//	s_log("[LOG] lock callback:missing dserv:%d", i);
			continue;
		}
		s_servg_rpc_call(serv, pkt, c, write_callback, -1);
	}

	s_packet_drop(pkt);
}

static void write_callback(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_glock_client * c = (struct s_glock_client *)ud;
	struct s_core * core = c->core;

	int type = s_servg_get_type(serv);
	int id = s_servg_get_id(serv);

	assert(type == S_SERV_TYPE_D);

	c->lock &= ~( 1 << id );

	//s_log("write callback now:%d.%d, from:%d, lock:%x", c->id.x, c->id.y, id, c->lock);

	if(c->lock == 0) {
	//	s_log("[LOG] write finished! unlock:%d.%d", c->id.x, c->id.y);

		int sz = s_packet_size_for_id(0);
		pkt = s_packet_easy(S_PKT_TYPE_GLOBAL_UNLOCK, sz);
		s_packet_write_int(pkt, c->id.x);
		s_packet_write_int(pkt, c->id.y);

		struct s_server * mserv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, 1);
		if(!mserv) {
			s_log("[LOG] missing mserv!");
			return;
		}

		s_servg_send(mserv, pkt);

		s_packet_drop(pkt);

		s_free(c);

		struct timeval tv, ret;
		gettimeofday(&tv, NULL);
		timersub(&tv, &c->tv, &ret);
	//	s_log("[LOG] id(%d.%d) time comsume: %lu.%lu", c->id.x, c->id.y, ret.tv_sec, ret.tv_usec);

		timersub(&tv, &g_tv_start, &ret);
	
		static int C = 0;
		C++;
		if(C >= MaxTry) {
			s_log("[LOG] total time consume: %lu.%lu", ret.tv_sec, ret.tv_usec);
		}

	}
}


#include "s_server.h"
#include "s_access_id.h"

static void lock_callback(struct s_zoo * z, void * d, struct s_zoo_lock_vector * v);
static void write_callback(struct s_server * serv, struct s_packet * pkt, void * udata);

struct s_glock_client {
	struct s_core * core;

	struct s_zoo * zoo;

	struct s_id id;
	unsigned int lock;

	int idx;

	struct s_zoo_lock_vector * v;

	struct timeval tv;
};

static int MaxTry = 0;
static int ProcessNum = 0;

static struct timeval gtv;
static int gid;
static int ginit = 0;
static int gcount = 0;

static struct s_array * g_result;

static struct s_zoo * g_zoo;
static struct s_array * g_access_id;

static void access_data(struct s_core * core)
{
	static char buf[256];

	gcount++;

	int idx = (core->id - 1) * 100 + gcount;

	struct s_array ** pp = s_array_at(g_access_id, idx);
	struct s_array * D = *pp;

	struct s_zoo_lock_vector * v = s_zoo_lockv_create(g_zoo);

	struct s_glock_client * c = s_malloc(struct s_glock_client, 1);
	c->core = core;
	c->id.x = core->id;
	c->id.y = gid++;
	c->v = v;
	c->lock = 0;
	c->idx = idx;

	gettimeofday(&c->tv, NULL);

	v->__id = c->id.y;
	v->__id2 = idx + 1;

	int i;
	for(i = 0; i < s_array_len(D); ++i) {
		int * pd = s_array_at(D, i);
		int d = *pd;

		sprintf(buf, "%d", d);
		
		s_zoo_lockv_add(v, buf);

		c->lock |= 1 << d;
	}

	s_zoo_lockv(g_zoo, v, &lock_callback, c);
}


void * s_ud_client_init(struct s_core * core)
{
	gid = 0;
	ginit = 1;

	g_result = s_array_create(sizeof(struct timeval), 16);

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

	ProcessNum = s_config_read_i(config, "ProcessNum");

	if(ProcessNum <= 0) {
		s_log("select config.default.ProcessNum error!");
		exit(0);
	}

	s_log("[LOG] MaxTry:%d, ProcessNum:%d", MaxTry, ProcessNum);

	// read access_id
	g_access_id = s_access_id_create("access_id.conf");
	if(!g_access_id) {
		s_log("Read AccessID.conf error!");
		exit(0);
	}

	g_zoo = s_zoo_init("114.212.143.234:2181");

	s_log("[LOG] Start Sync ...");

	s_zoo_sync(g_zoo, ProcessNum, "test2");

	gettimeofday(&gtv, NULL);
	s_log("[LOG] Sync OK, %ld s, %ld us", gtv.tv_sec, gtv.tv_usec);

	// access the first data

	int i;
	for(i = 0; i < MaxTry; ++i) {
		access_data(core);
	}

	return NULL;
}

void s_ud_client_update(struct s_core * core, void * ud)
{
}
/*

void s_ud_client_update(struct s_core * core, void * ud)
{
	if(!ginit) {
		return;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
label_loop:

	if(gcount >= MaxTry) {
		ginit = 0;
		gettimeofday(&g_tv_start, NULL);
		return;
	}

//	struct timeval tv;
//	gettimeofday(&tv, NULL);

//	struct timeval ret;
//	timersub(&tv, &gtv, &ret);

//	if(ret.tv_usec < 0) {
//		return;
//	}

//	gtv = tv;

	struct s_packet * pkt;
	struct s_glock_client * c = s_malloc(struct s_glock_client, 1);

	c->id.x = core->id;
	c->id.y = gid++;
	c->tv = tv;
	c->lock = 0;
	c->core = core;


	int servs[32];
	int nserv = 0;

	nserv = irand_serv(servs);

	int i;
	for(i = 0; i < 5; ++i) {
		int serv_id = servs[i];
		if(serv_id > 0) {
			c->lock |= 1 << serv_id;
		}
	}

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
goto label_loop;
}
*/

static void lock_callback(struct s_zoo * z, void * ud,  struct s_zoo_lock_vector * v)
{
	static int C = 0;
	C++;

	struct s_glock_client * c = ud;
	struct s_core * core = c->core;

	int sz = s_packet_size_for_id(0);
	struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_WRITE, sz);

	s_packet_write_int(pkt, c->id.x);
	s_packet_write_int(pkt, c->id.y);

	int idx = c->idx;

	struct s_array ** pp = s_array_at(g_access_id, idx);
	struct s_array * D = *pp;

	int i;
	for(i = 1; i < 32; ++i) {

		if((c->lock & (1 << i)) == 0) {
			continue;
		}

		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_D, i);
		if(!serv) {
			s_log("[LOG] lock callback:missing dserv:%d", i);
			continue;
		}
		s_servg_rpc_call(serv, pkt, c, write_callback, -1);
	}

	s_packet_drop(pkt);
}

static void write_callback(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_glock_client * c = ud;
	struct s_core * core = c->core;

	int type = s_servg_get_type(serv);
	int id = s_servg_get_id(serv);

	assert(type == S_SERV_TYPE_D);

	c->lock &= ~(1 << id);

	if(c->lock == 0) {

		// unlock
		s_zoo_unlockv(g_zoo, c->v);

		struct timeval tv, ret;
		gettimeofday(&tv, NULL);
		timersub(&tv, &gtv, &ret);

		while(s_array_len(g_result) <= c->id.y) {
			s_array_push(g_result);
		}

		struct timeval * p = s_array_at(g_result, c->id.y);
		p->tv_sec = ret.tv_sec;
		p->tv_usec = ret.tv_usec;

		static int C = 0;
		C++;
		if(C >= MaxTry) {
			// write to file
			char fname[256];
			sprintf(fname, "./result.%d", core->id);
			int file = open(fname, O_RDWR | O_CREAT, 0x777);
			int i;
			for(i = 0; i < s_array_len(g_result); ++i) {
				struct timeval * p = s_array_at(g_result, i); 
				char buf[1024];
				sprintf(buf, "%ld\n", p->tv_sec * 1000000 + p->tv_usec);
				write(file, buf, strlen(buf));
			}   
			close(file);

			s_log("All End!");

		}
	}
}


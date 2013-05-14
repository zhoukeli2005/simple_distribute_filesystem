#include "s_server.h"
#include "s_access_id.h"

static void lock_callback(struct s_server * serv, struct s_packet * pkt, void * udata);

struct s_glock_client {
	struct s_core * core;

	struct s_zoo * z;

	struct s_id id;

	struct timeval tv;
};

static int MaxTry = 0;
static int ProcessNum = 0;

static struct timeval gtv;
static struct timeval g_tv_start;

static int gid;
static int ginit = 0;
static int gcount = 0;

static struct s_zoo * g_zoo;
static struct s_array * g_access_id;

static struct s_array * g_result;


/*
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
	i = 0;

	memcpy(out, serv[i], sizeof(int) * 5);

	return 3;
}*/

static void access_data(struct s_core * core)
{
	gcount++;
	if(gcount > MaxTry) {
		return;
	}

	struct s_packet * pkt;
	struct s_glock_client * c = s_malloc(struct s_glock_client, 1);

	c->id.x = core->id;
	c->id.y = gid++;
	c->core = core;

	gettimeofday(&c->tv, NULL);

	int idx = (core->id - 1) * 100 + gcount;

	struct s_array ** pp = s_array_at(g_access_id, idx);
	struct s_array * D = *pp;

	int i;
	int nserv = s_array_len(D);

	int sz = s_packet_size_for_int(0) +
		s_packet_size_for_int(0) +
		s_packet_size_for_int(0) +
	       	s_packet_size_for_id(0) +
		( s_packet_size_for_int(0) + 
		  s_packet_size_for_id(0)
		) * nserv;

	pkt = s_packet_easy(S_PKT_TYPE_LOCK_START, sz);

	s_packet_write_int(pkt, 0);
	s_packet_write_int(pkt, nserv);
	s_packet_write_int(pkt, core->id);
	s_packet_write_int(pkt, c->id.x);
	s_packet_write_int(pkt, c->id.y);
	for(i = 0; i < nserv; ++i) {
		int * pd = s_array_at(D, i);
		int d = *pd;
		s_packet_write_int(pkt, d);
		s_packet_write_int(pkt, c->id.x);
		s_packet_write_int(pkt, c->id.y);
	}

	int *pd = s_array_at(D, 0);
	int d = *pd;

	struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_D, d);
	if(serv) {
		s_servg_rpc_call(serv, pkt, c, &lock_callback, -1);

		struct s_client * cl = s_core_client(core);
		struct s_glock_client ** pp = s_hash_set_id(cl->locks, c->id);
		*pp = c;

	} else {
		s_log("[Warning] no dserv:%d", d);
	}

	s_packet_drop(pkt);
}

void * s_ud_client_init(struct s_core * core)
{
	gid = 0;
	ginit = 1;

	g_result = s_array_create(sizeof(struct timeval), 16);

	s_servg_register(core->servg, S_SERV_TYPE_D, S_PKT_TYPE_LOCK_END, &lock_callback);

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

	// read access id
	g_access_id = s_access_id_create("access_id.conf");
	if(!g_access_id) {
		s_log("Read AccessID.conf error!");
		exit(0);
	}

	g_zoo = s_zoo_init("114.212.143.234:2181");
	s_log("[LOG] Start Sync ...");

	s_zoo_sync(g_zoo, ProcessNum, "test3");

	gettimeofday(&gtv, NULL);
	s_log("[LOG] Sync OK, %ld s %ld us", gtv.tv_sec, gtv.tv_usec);

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
	c->core = core;

	int i;
	int nserv = 0;
	int servs[32];
	nserv = irand_serv(servs);

	int sz = s_packet_size_for_int(0) +
		s_packet_size_for_int(0) +
		s_packet_size_for_int(0) +
	       	s_packet_size_for_id(0) +
		( s_packet_size_for_int(0) + 
		  s_packet_size_for_id(0)
		) * nserv;

	pkt = s_packet_easy(S_PKT_TYPE_LOCK_START, sz);

	s_packet_write_int(pkt, 0);
	s_packet_write_int(pkt, nserv);
	s_packet_write_int(pkt, core->id);
	s_packet_write_int(pkt, c->id.x);
	s_packet_write_int(pkt, c->id.y);
	for(i = 0; i < nserv; ++i) {
		s_packet_write_int(pkt, servs[i]);
		s_packet_write_int(pkt, c->id.x);
		s_packet_write_int(pkt, c->id.y);
	}

	struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_D, servs[0]);
	if(serv) {
		s_servg_rpc_call(serv, pkt, c, &lock_callback, -1);

		struct s_client * cl = s_core_client(core);
		struct s_glock_client ** pp = s_hash_set_id(cl->locks, c->id);
		*pp = c;

	} else {
		s_log("[Warning] no dserv:%d", servs[0]);
	}

	s_packet_drop(pkt);

	gcount++;
//	if(gcount == 1) {
//		g_tv_start = tv;
//	}

	goto label_loop;
}*/

static void lock_callback(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	if(!pkt) {
		return;
	}
	struct s_core * core = ud;
	struct s_client * cl = s_core_client(core);

	struct s_id id;
	s_packet_read(pkt, &id.x, int, label_error);
	s_packet_read(pkt, &id.y, int, label_error);

	struct s_glock_client ** pp = s_hash_get_id(cl->locks, id);
	if(!pp) {
		s_log("[Warning] lock callback, hash_get_id failed!");
		return;
	}

	struct s_glock_client * c = *pp;

	struct timeval tv, ret;
	gettimeofday(&tv, NULL);
	timersub(&tv, &c->tv, &ret);
	timersub(&tv, &gtv, &ret);

	s_log("%d", id.y);
	while(s_array_len(g_result) <= id.y) {
		s_array_push(g_result);
	}

	struct timeval * p = s_array_at(g_result, id.y);
	p->tv_sec = ret.tv_sec;
	p->tv_usec = ret.tv_usec;


//	s_log("[Time Comsume] (%d,%d) %ld s %ld us", c->id.x, c->id.y, ret.tv_sec, ret.tv_usec);

	static int C = 0;
	C++;
	s_log("C:%d", C);

	if(C >= MaxTry) {
		// write to file
		char fname[256];
		sprintf(fname, "./result.%d", core->id);
		int file = open(fname, O_RDWR | O_CREAT, 0x777);
		int i;
		for(i = 0; i < s_array_len(g_result); ++i) {
			struct timeval * p = s_array_at(g_result, i);
			char buf[1024];
			sprintf(buf, "%ld\n",p->tv_sec * 1000000 + p->tv_usec);
			write(file, buf, strlen(buf));
		}
		close(file);

		s_log("All End!");
	}

	return;

label_error:
	s_log("[Warning] lock_callback : read error pkt!");
	return;
}


#include "s_server.h"

static void lock_callback(struct s_server * serv, struct s_packet * pkt, void * udata);

struct s_glock_client {
	struct s_core * core;

	struct s_id id;

	struct timeval tv;
};

static struct timeval gtv;
static int gid;
static int ginit = 0;
static int gcount = 0;

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

	return 0;
}

void * s_ud_client_init(struct s_core * core)
{
	gettimeofday(&gtv, NULL);
	gid = 0;
	ginit = 1;

	s_servg_register(core->servg, S_SERV_TYPE_D, S_PKT_TYPE_LOCK_END, &lock_callback);

	return NULL;
}

void s_ud_client_update(struct s_core * core, void * ud)
{
	if(!ginit) {
		return;
	}

	if(gcount >= 50) {
		return;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct timeval ret;
	timersub(&tv, &gtv, &ret);

	if(ret.tv_usec < 100) {
		return;
	}

	gtv = tv;

	struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_PUSH_DATA, 3 * 1024 * 1024);
	struct s_glock_client * c = s_malloc(struct s_glock_client, 1);

	c->id.x = core->id;
	c->id.y = gid++;
	c->tv = tv;
	c->core = core;

	s_log("start to write id(%d,%d) ...", c->id.x, c->id.y);

	s_packet_write_int(pkt, c->id.x);
	s_packet_write_int(pkt, c->id.y);

	int nserv = 0;
	int servs[32];
	irand_serv(servs);

	int i;
	for(i = 0; i < 5; ++i) {
		int serv_id = servs[i];
		if(!serv_id) {
			break;
		}
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_D, serv_id);
		if(!serv) {
			s_log("[LOG] missing dserv:%d", serv_id);
			continue;
		}
		s_servg_send(serv, pkt);
		nserv++;
	}

	s_packet_drop(pkt);

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
}

static void lock_callback(struct s_server * serv, struct s_packet * pkt, void * ud)
{
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

	s_log("[LOG] id(%d.%d) time consume : %lu.%lu", id.x, id.y, ret.tv_sec, ret.tv_usec);
	return;

label_error:
	s_log("[Warning] lock_callback : read error pkt!");
	return;
}

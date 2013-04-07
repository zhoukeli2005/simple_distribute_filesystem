#include "s_core.h"
#include "s_core_param.h"
#include "s_core_create.h"
#include "s_core_glock.h"
#include "s_core_dserv.h"
#include "s_core_lock.h"
#include "s_core_paxos.h"

#include <s_mem.h>

static struct s_core g_core;

static void init_client( struct s_core * core );
static void init_mserv( struct s_core * core );
static void init_dserv( struct s_core * core );

static void when_serv_established(struct s_server * serv, void * ud);
static void when_serv_closed(struct s_server * serv, void * ud);
static void when_all_established(void * ud);

struct s_core * s_core_create( struct s_core_create_param * param )
{
	struct s_core * core = &g_core;
	core->type = param->type;
	core->id = param->id;

	memcpy(&core->create_param, param, sizeof(struct s_core_create_param));

	struct s_servg_callback callback = {
		.udata = core,
		.serv_established = when_serv_established,
		.serv_closed = when_serv_closed,
		.all_established = when_all_established
	};

	core->servg = s_servg_create(core->type, core->id, &callback);
	if(!core->servg) {
		s_log("[Error] server-group create failed!");
		return NULL;
	}

	if(s_servg_init_config(core->servg, param->config) < 0) {
		s_log("[Error] server-group init config failed!");
		return NULL;
	}

	core->paxos = s_paxos_create(core);

	int type = core->type;

	if(type == S_SERV_TYPE_C) {
		// client
		init_client(core);
	}

	if(type == S_SERV_TYPE_M) {
		// mserv
		init_mserv(core);
	}

	if(type == S_SERV_TYPE_D) {
		// dserv
		init_dserv(core);
	}

	// init core param
	s_core_param_init();

	return core;
}

int s_core_poll(struct s_core * core, int msec)
{
	int r = s_servg_poll(core->servg, msec);
	if(r < 0) {
		return -1;
	}

	// XXX do something ...
	if(core->create_param.update) {
		core->create_param.update(core, core->create_param.ud);
	}

	return 0;
}

static void init_client( struct s_core * core )
{
	struct s_client * cl = s_core_client(core);
	cl->locks = s_hash_create(sizeof(void *), 16);
}

static void init_mserv( struct s_core * core )
{
	struct s_mserver * mserv = s_core_mserv(core);
	mserv->file_creating = s_hash_create(sizeof(struct s_core_mcreating), 16);
	mserv->file_metadata = s_hash_create(sizeof(struct s_file_meta_data), 16);
	mserv->file_meta_metadata = s_hash_create(sizeof(struct s_file_meta_meta_data *), 16);

	// glock
	mserv->glock = 0;
	mserv->glock_elems = s_hash_create(sizeof(struct s_glock_elem), 16);


	/* ------ glock ---------- */
	s_servg_register(core->servg, S_SERV_TYPE_C, S_PKT_TYPE_GLOBAL_LOCK, &s_core_mserv_glock);
	s_servg_register(core->servg, S_SERV_TYPE_C, S_PKT_TYPE_GLOBAL_UNLOCK, &s_core_mserv_glock_unlock);


	/* ------- create file --------- */
	// client rpc
	s_servg_register(core->servg, S_SERV_TYPE_C, S_PKT_TYPE_CREATE, &s_core_mserv_create);

	// mserv rpc
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_CHECK_AUTH,	&s_core_mserv_create_check_auth);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_METADATA,	&s_core_mserv_create_metadata);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_META_META_DATA,	&s_core_mserv_create_meta_meta_data);
}

static void init_dserv( struct s_core * core )
{
	struct s_dserver * dserv = s_core_dserv(core);
	dserv->waiting_data = s_hash_create(sizeof(struct s_core_wait_data), 16);

	// lock
	dserv->locks = s_hash_create(sizeof(void *), 16);
	s_list_init(&dserv->lock_sent);
	s_list_init(&dserv->lock_waiting);

	s_servg_register(core->servg, S_SERV_TYPE_C, S_PKT_TYPE_LOCK_START, &s_core_dserv_lock);
	s_servg_register(core->servg, S_SERV_TYPE_D, S_PKT_TYPE_LOCK_NEXT, &s_core_dserv_lock);
	s_servg_register(core->servg, S_SERV_TYPE_D, S_PKT_TYPE_LOCK_UNLOCK, &s_core_dserv_lock_unlock);

	
	/* ------ create file ------ */
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_METADATA, &s_core_dserv_create_metadata);

	/* ------ write data ------- */
	s_servg_register(core->servg, S_SERV_TYPE_C, S_PKT_TYPE_PUSH_DATA, &s_core_dserv_push_data);
	s_servg_register(core->servg, S_SERV_TYPE_C, S_PKT_TYPE_WRITE, &s_core_dserv_write);
}

static void when_serv_established(struct s_server * serv, void * ud)
{
	struct s_core * core = (struct s_core *)ud;

	s_used(core);
	s_used(serv);
}

static void when_serv_closed(struct s_server * serv, void * ud)
{
	struct s_core * core = (struct s_core *)ud;

	s_used(core);
	s_used(serv);
}

static void when_all_established(void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	if(core->create_param.all_established) {
		core->create_param.ud = core->create_param.all_established(core);
	}
}

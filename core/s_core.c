#include "s_core.h"
#include "s_core_create.h"

static struct s_core g_core;

static void init_client( struct s_core * core );
static void init_mserv( struct s_core * core );
static void init_dserv( struct s_core * core );

static void when_serv_established(struct s_server * serv, void * ud);
static void when_serv_closed(struct s_server * serv, void * ud);

struct s_core * s_core_create( int type, int id, struct s_config * config )
{
	struct s_core * core = &g_core;
	core->type = type;
	core->id = id;

	struct s_servg_callback callback = {
		.udata = core,
		.serv_established = when_serv_established,
		.serv_closed = when_serv_closed
	};

	core->servg = s_servg_create(type, id, &callback);
	if(!core->servg) {
		s_log("[Error] server-group create failed!");
		return NULL;
	}

	if(s_servg_init_config(core->servg, config) < 0) {
		s_log("[Error] server-group init config failed!");
		return NULL;
	}

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

	return core;
}

int s_core_poll(struct s_core * core, int msec)
{
	int r = s_servg_poll(core->servg, msec);
	if(r < 0) {
		return -1;
	}

	// XXX do something ...

	return 0;
}

static void init_client( struct s_core * core )
{
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_BACK, &s_core_client_create_back);
}

static void init_mserv( struct s_core * core )
{
	s_servg_register(core->servg, S_SERV_TYPE_C, S_PKT_TYPE_CREATE, &s_core_mserv_create);

	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_CHECK_AUTH, &s_core_mserv_create_check_auth);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_CHECK_AUTH_BACK, &s_core_mserv_create_check_auth_back);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_DECIDE, &s_core_mserv_create_decide);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_CANCEL, &s_core_mserv_create_cancel);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_METADATA, &s_core_mserv_create_metadata);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_METADATA_ACCEPT, &s_core_mserv_create_md_accept);

	s_servg_register(core->servg, S_SERV_TYPE_D, S_PKT_TYPE_CREATE_METADATA_ACCEPT, &s_core_mserv_create_md_accept);
}

static void init_dserv( struct s_core * core )
{
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_CREATE_METADATA, &s_core_dserv_create_metadata);
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

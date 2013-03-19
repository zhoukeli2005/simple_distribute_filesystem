#include "s_core.h"
#include "s_core_param.h"
#include "s_core_create.h"

static void irpc_check_auth_back(struct s_conn * conn, void * d);

static int icheck_can_create(struct s_core * core, int from_mserv_id, struct s_string * fname)
{
	struct s_mserver * mserv = s_core_mserv(core);
	struct s_core_mcreating * creating = s_hash_get_str(mserv->file_creating, fname);
	if(!creating) {
		// not creating.. check file-meta-meta-data
		struct s_file_meta_meta_data * mmd = s_hash_get_str(mserv->file_meta_metadata, fname);
		if(mmd && mmd->flags != S_FILE_MMD_FLAG_DELETED) {
			// already exist
			return 0;
		}
		return 1;
	}
	if(from_mserv_id <= core->id) {
		return 1;
	}
	return 0;
}

static void icreate_fmmd(struct s_core * core, struct s_core_mcreating * creating)
{
	struct s_mserver * mserv = s_core_mserv(core);

	/* 1. select mservs -- min mem used */
	int nmserv = 1;
	unsigned int min_mem = 0;
	int mserv_id = -1;
	int id = -1;
	struct s_server * serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_M, &id);
	while(serv) {
		if(s_servg_get_id(serv) == core->id) {
			continue;
		}
		unsigned int mem = s_servg_get_mem(serv);
		if(min_mem == 0 || mem < min_mem) {
			min_mem = mem;
			mserv_id = s_servg_get_id(serv);
			nmserv++;
		}
		serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_M, &id);
	}

	/* 2. create file-desc */
	struct s_file_desc fdesc = { 0 };
	struct s_file_meta_meta_data ** pmmd = s_hash_get_str(mserv->file_meta_metadata, creating->fname);
	struct s_file_meta_meta_data * mmd;
	if(pmmd) {
		mmd = *pmmd;
		assert(mmd->flags == S_FILE_MMD_FLAG_DELETED);
		fdesc.file_id = mmd->fdesc.file_id;
		fdesc.file_version = mmd->fdesc.file_version + 1;

		s_file_meta_data_destroy(mmd);
	} else {
		fdesc.file_id = (core->id << 16 && 0xFFFF0000) | (core->file_id++ & 0xFFFF);
		fdesc.file_version = 1;
	}

	s_log("[LOG] create file(id:%x, version:%u)", fdesc.file_id, fdesc.file_version);

	/* 3. create new file-meta-meta-data */
	mmd = s_file_meta_meta_data_create( core, nmserv );
	mmd->mservs[0] = core->id;
	if(nmserv > 1) {
		mmd->mservs[1] = mserv_id;
	}

	mmd->fname = s_string_grab(creating->fname);

	/* 4. save in file_mmd hash */
	pmmd = s_hash_set_str(mserv->file_meta_metadata, creating->fname);
	*pmmd = mmd;

	/* 5. broadcast file-meta-meta-data */
	int sz = s_packet_size_for_string(creating->fname) +
		s_packet_size_for_uint(fdesc.file_id) +
		s_packet_size_for_uint(fdesc.file_version) +
		s_packet_size_for_uint(fdesc.nmserv) +
		s_packet_size_for_uint() * fdesc.nmserv;

	struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_CREATE_META_META_DATA, sz);
	s_packet_write_string(pkt, creating->fname);
	s_packet_write_uint(pkt, fdesc.file_id);
	s_packet_write_uint(pkt, fdesc.file_version);
	s_packet_write_uint(pkt, fdesc.nmserv);
	int i;
	for(i = 0; i < fdesc.nmserv; ++i) {
		s_packet_write_uint(pkt, fdesc.mservs[i]);
	}

	int id = -1;
	struct s_server * serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_M, &id);
	while(serv) {
		s_net_send(s_servg_get_conn(serv), pkt);
		serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_M, &id);
	}

	s_packet_drop(pkt);
}

static void icreate_fmd(struct s_core * core, struct s_core_mcreating * creating)
{
	/* 1. find top N min-mem data-serv */
	unsigned int dserv[2] = {0};
	unsigned int dmem[2] = {0};

	int id = -1;
	struct s_server * serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_D, &id);
	while(serv) {
		unsigned int mem = s_servg_get_mem(serv);
		if(dmem[0] == 0) {
			dmem[0] = mem;
			dserv[0] = s_servg_get_id(serv);
		} else if (dmem[1] == 0 || mem < dmem[1]) {
			int i = 1;
			if(mem < dmem[0]) {
				dmem[1] = dmem[0];
				dserv[1] = dserv[0];
				i = 0;
			}
			dmem[i] = mem;
			dserv[i] = s_servg_get_id(serv);
		}
	}
}

static void icreate(struct s_core * core, struct s_core_mcreating * creating)
{
	creating->ncheck = 0;

	/* 1. create file-meta-meta-data and broadcast */
	icreate_fmmd(core, creating);
	
	/* 2. create file-meta-data and send to related servs */
	icreate_fmd(core, creating);
}

void s_core_mserv_create(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	unsigned int req_id = s_packet_get_req(pkt);
	struct s_string * fname = NULL;
	struct s_file_size size;

	s_packet_read(pkt, &fname, string, label_error);
	s_packet_read(pkt, &size.low, uint, label_error);
	s_packet_read(pkt, &size.high, uint, label_error);

	s_log("[LOG] create file : %s, size:%u-%u, req_id:%u", s_string_data_p(fname), size.high, size.low, req_id);

	struct s_core * core = (struct s_core *)ud;
	struct s_mserver * mserv = s_core_mserv(core);
	struct s_conn * conn = s_servg_get_conn(serv);

	if(!icheck_can_create(core, s_servg_get_id(serv), fname)) {
		s_log("[LOG] cannot create! already create in this mserver!");
		goto label_error;
	}

	struct s_core_mcreating * creating = s_hash_set_str(mserv->file_creating, fname);
	creating->fname = s_string_grab(fname);
	creating->core = core;
	creating->client = serv;
	creating->ncheck = 0;
	creating->size = size;

	// check all other mservs
	int sz = s_packet_size_for_string(fname);
	struct s_packet * pkt_ask = s_packet_easy(S_PKT_TYPE_CREATE_CHECK_AUTH, sz);
	s_packet_write_string(pkt_ask, fname);

	int id = -1;
	struct s_server * serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_M, &id);
	while(serv) {
		if(s_servg_get_id(serv) != core->id) {
			s_log("[LOG] ask mserv:%d", s_servg_get_id(serv));
			creating->ncheck++;

			// send check-auth-request
			s_net_rpc_call(s_servg_get_conn(serv), pkt_ask, s_string_grab(fname), irpc_check_auth_back);
		}
		serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_M, &id);
	}

	s_packet_drop(pkt_ask);

	if(creating->ncheck == 0) {
		// wait for no mserv, decide on my own
		s_log("[LOG] decide my own");
		icreate(core, creating);
	}

	s_string_drop(fname);
	return;


label_error:
	{
		if(fname) {
			s_string_drop(fname);
		}
		struct s_packet * pkt_ret = s_packet_create(4);
		s_packet_write_int(0);
		s_net_rpc_ret(conn, req_id, pkt_ret);
		s_packet_drop(pkt_ret);
	}

	return;
}

void irpc_check_auth_back(struct s_conn * conn, struct s_packet * pkt, void * ud)
{
	struct s_core * core = s_servg_gdata_from_conn(conn);
	struct s_string * fname = (struct s_string *)ud;
	struct s_mserver * mserv = s_core_mserv(core);

	struct s_core_mcreating * creating = s_hash_get_str(mserv->file_creating, fname);
	if(!creating) {
		s_log("[Warning] check_auth_back(file:%s), not exist!", s_string_data_p(fname));
		s_string_drop(fname);
		return;
	}
	int ret;
	if(s_packet_read_int(pkt, &ret) < 0) {
		s_log("[Error] check_auth_back(file:%s), miss ret!", s_string_data_p(fname));
		goto label_check_failed;
	}
	if(ret <= 0) {
		goto label_check_failed;
	}

	creating->ncheck--;
	if(creating->ncheck == 0) {
		// ok
		icreate(core, creating);
	}

	s_string_drop(fname);

	return;

label_check_failed:
	{
		s_string_drop(fname);
		s_string_drop(creating->fname);
		s_hash_del_str(mserv->file_creating, fname);
	}
}

void s_core_mserv_create_check_auth(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	struct s_mserver * mserv = s_core_mserv(core);
	struct s_string * fname;

	s_packet_read(pkt, &fname, label_error);

	int ret = icheck_can_create(core, s_servg_get_id(serv), fname);

	struct s_packet * pkt_ret = s_packet_create(4);
	s_packet_write(pkt_ret, ret);
	s_net_rpc_ret(s_servg_get_conn(serv), s_packet_get_req(pkt), pkt_ret);
	s_packet_drop(pkt_ret);

	return;

label_error:

	return;
}

void s_core_mserv_create_metadata(struct s_server * serv, struct s_packet * pkt, void * ud)
{
}

void s_core_mserv_create_meta_metadata(struct s_server * serv, struct s_packet * pkt, void * ud)
{
}

void s_core_mserv_create_md_accept(struct s_server * serv, struct s_packet * pkt, void * ud)
{
}


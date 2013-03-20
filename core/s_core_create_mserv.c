#include "s_core.h"
#include "s_core_param.h"
#include "s_core_create.h"
#include "s_core_pkt.h"
#include "s_core_util.h"

static void irpc_check_auth_back(struct s_server * serv, struct s_packet * pkt, void * d);
static void irpc_send_md_back(struct s_server * serv, struct s_packet * pkt, void * d);

static void icreate_fail_to_client(struct s_core * core, unsigned int req_id, unsigned int client_id)
{
	struct s_server * client = s_servg_get_active_serv(core->servg, S_SERV_TYPE_C, client_id);
	if(!client) {
		s_log("[Warning] create_fail, client(%d) is missing!", client_id);
		return;
	}

	struct s_packet * pkt_ret = s_packet_create(4);
	s_packet_write_int(pkt_ret, 0);
	s_servg_rpc_ret(client, req_id, pkt_ret);
	s_packet_drop(pkt_ret);
}

static void icreate_suc_to_client(struct s_core * core, struct s_core_mcreating * creating, struct s_file_meta_data * md)
{
	struct s_server * client = s_servg_get_active_serv(core->servg, S_SERV_TYPE_C, creating->client_id);
	if(!client) {
		s_log("[Warning] create_suc(%s), client(%d) is missing!", s_string_data_p(creating->fname), creating->client_id);
		return;
	}

	struct s_packet * pkt = s_core_pkt_from_fmd(core, md);
	s_packet_set_fun(pkt, S_PKT_TYPE_CREATE_METADATA);
	s_servg_rpc_ret(client, creating->req_id, pkt);
	s_packet_drop(pkt);
}

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
	if(from_mserv_id < core->id) {
		return 1;
	}
	return 0;
}

static struct s_file_meta_meta_data * icreate_fmmd(struct s_core * core, struct s_core_mcreating * creating)
{
	struct s_mserver * mserv = s_core_mserv(core);

	/* 1. select mservs -- min mem used

	   	notice : the mserv which client sent requets always contain the file-meta-data
	 */
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

		s_file_meta_meta_data_destroy(mmd);
	} else {
		fdesc.file_id = (core->id << 16 && 0xFFFF0000) | (core->file_id++ & 0xFFFF);
		fdesc.file_version = 1;
	}

	s_log("[LOG] create file(id:%x, version:%u)", fdesc.file_id, fdesc.file_version);

	/* 3. create new file-meta-meta-data */
	{
		mmd = s_file_meta_meta_data_create( core, nmserv );
		mmd->fdesc = fdesc;
		mmd->fname = s_string_grab(creating->fname);

		mmd->nmserv = nmserv;
		mmd->mservs[0] = core->id;
		if(nmserv > 1) {
			mmd->mservs[1] = mserv_id;
		}
	}

	/* 4. save in file_mmd hash */
	pmmd = s_hash_set_str(mserv->file_meta_metadata, creating->fname);
	*pmmd = mmd;

	/* 5. broadcast file-meta-meta-data */
	{
		struct s_packet * pkt = s_core_pkt_from_fmmd(core, mmd);

		int id = -1;
		struct s_server * serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_M, &id);
		while(serv) {
			s_servg_send(serv, pkt);
			serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_M, &id);
		}

		s_packet_drop(pkt);
	}

	return mmd;
}

static struct s_file_meta_data * icreate_fmd(struct s_core * core, struct s_file_meta_meta_data * mmd, struct s_core_mcreating * creating)
{
	struct s_mserver * mserv = s_core_mserv(core);

	/* 1. find top N min-mem data-serv : By now N == 2 */
	int ndserv = 0;

	#define MAX_D 2

	unsigned int dserv[MAX_D] = {0};
	unsigned int dmem[MAX_D] = {0};

	int id = -1;
	struct s_server * serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_D, &id);
	while(serv) {
		unsigned int mem = s_servg_get_mem(serv);
		if(dmem[0] == 0) {
			ndserv++;

			dmem[0] = mem;
			dserv[0] = s_servg_get_id(serv);
		} else if (dmem[1] == 0 || mem < dmem[1]) {
			ndserv++;

			int i = 1;
			if(mem < dmem[0]) {
				dmem[1] = dmem[0];
				dserv[1] = dserv[0];
				i = 0;
			}
			dmem[i] = mem;
			dserv[i] = s_servg_get_id(serv);
		}

		serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_D, &id);
	}

	ndserv = s_min(ndserv, MAX_D);

	if(ndserv == 0) {
		s_log("[Error] create file:%s, no dserver!", s_string_data_p(mmd->fname));
	}

	/* 2. create file-meta-data */
	int nblocks = s_core_fsize_to_block_num(&creating->size);
	struct s_file_meta_data * fmd = s_file_meta_data_create( core, nblocks );
	fmd->fname = s_string_grab(mmd->fname);
	fmd->fdesc = mmd->fdesc;
	fmd->fsize = creating->size;
	fmd->nblocks = nblocks;

	s_log("[LOG] create file-meta-data: size:%u-%u, nblocks:%d", fmd->fsize.high, fmd->fsize.low, fmd->nblocks);
	int i;
	for(i = 0; i < nblocks; ++i) {
		fmd->blocks[i].locate[0] = dserv[0];
		fmd->blocks[i].locate[1] = dserv[1];
	}

	/* 3. save file-meta-data in this mserv */
	struct s_file_meta_data ** ppmd = s_hash_set_str(mserv->file_metadata, mmd->fname);
	*ppmd = fmd;

	/* 4. send to related mserv & dserv */
	struct s_packet * pkt = s_core_pkt_from_fmd(core, fmd);
	s_packet_set_fun(pkt, S_PKT_TYPE_CREATE_METADATA);
	for(i = 0; i < mmd->nmserv; ++i) {
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, mmd->mservs[i]);
		if(serv && s_servg_get_id(serv) != core->id) {
			s_servg_rpc_call(serv, pkt, s_string_grab(creating->fname), &irpc_send_md_back, 3000);
			creating->ncheck++;
		}
	}
	for(i = 0; i < ndserv; ++i) {
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_D, dserv[i]);
		if(serv) {
			s_servg_rpc_call(serv, pkt, s_string_grab(creating->fname), &irpc_send_md_back, 3000);
			creating->ncheck++;
		}
	}

	if(creating->ncheck == 0) {
		s_log("[Warning] send file-meta-data error : none serv!");
	}

	return fmd;
}

static void icreate(struct s_core * core, struct s_core_mcreating * creating)
{
	creating->ncheck = 0;

	/* 1. create file-meta-meta-data and broadcast */
	struct s_file_meta_meta_data * mmd = icreate_fmmd(core, creating);
	
	/* 2. create file-meta-data and send to related servs */
	struct s_file_meta_data * md = icreate_fmd(core, mmd, creating);
	s_used(md);
}

void s_core_mserv_create(struct s_server * client, struct s_packet * pkt, void * ud)
{
	unsigned int req_id = s_packet_get_req(pkt);
	unsigned int client_id = s_servg_get_id(client);
	struct s_string * fname = NULL;
	struct s_file_size size;

	s_packet_read(pkt, &fname, string, label_error);
	s_packet_read(pkt, &size.low, uint, label_error);
	s_packet_read(pkt, &size.high, uint, label_error);

	s_log("[LOG] create file : %s, size:%u-%u, req_id:%u", s_string_data_p(fname), size.high, size.low, req_id);

	struct s_core * core = (struct s_core *)ud;
	struct s_mserver * mserv = s_core_mserv(core);

	if(!icheck_can_create(core, core->id, fname)) {
		s_log("[LOG] cannot create! already create in this mserver!");
		goto label_failed;
	}

	struct s_core_mcreating * creating = s_hash_set_str(mserv->file_creating, fname);
	creating->fname = s_string_grab(fname);
	creating->core = core;
	creating->client_id = client_id;
	creating->req_id = req_id;
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
			s_servg_rpc_call(serv, pkt_ask, s_string_grab(fname), irpc_check_auth_back, 3000);
		}
		serv = s_servg_next_active_serv(core->servg, S_SERV_TYPE_M, &id);
	}

	s_packet_drop(pkt_ask);

	if(creating->ncheck == 0) {
		// wait for no mserv, decide myself
		s_log("[LOG] create: decide myself");
		icreate(core, creating);
	}

	s_string_drop(fname);
	return;


label_failed:
	icreate_fail_to_client(core, req_id, client_id);

label_error:

	if(fname) {
		s_string_drop(fname);
	}

	return;
}

void irpc_check_auth_back(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = s_servg_gdata_from_serv(serv);
	struct s_string * fname = (struct s_string *)ud;
	struct s_mserver * mserv = s_core_mserv(core);

	struct s_core_mcreating * creating = s_hash_get_str(mserv->file_creating, fname);
	if(!creating) {
		s_log("[Warning] check_auth_back(file:%s), not exist!", s_string_data_p(fname));
		s_string_drop(fname);
		return;
	}

	if(pkt) {
		int ret;
		if(s_packet_read_int(pkt, &ret) < 0) {
			s_log("[Error] check_auth_back(file:%s), miss ret!", s_string_data_p(fname));
			goto label_check_failed;
		}
		if(ret <= 0) {
			goto label_check_failed;
		}
	} else {
		// timeout or conn-broken
	}

	creating->ncheck--;
	if(creating->ncheck == 0) {
		// check ok
		icreate(core, creating);
	}

	s_string_drop(fname);

	return;

label_check_failed:
	{
		icreate_fail_to_client(core, creating->req_id, creating->client_id);

		s_string_drop(fname);
		s_string_drop(creating->fname);
		s_hash_del_str(mserv->file_creating, fname);
	}
}

static void irpc_send_md_back(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_string * fname = (struct s_string *)ud;
	if(!pkt) {
		// timeout or conn-broken
		s_log("[Warning] send meta-data to serv(%d-%d), timeout!", s_servg_get_type(serv), s_servg_get_id(serv));
	}

	struct s_core * core = s_servg_gdata_from_serv(serv);
	struct s_mserver * mserv = s_core_mserv(core);
	struct s_core_mcreating * creating = s_hash_get_str(mserv->file_creating, fname);
	if(!creating) {
		s_log("[Error] rpc_send_md_back, from:%d-%d, file:%s, no creating here!", s_servg_get_type(serv),
				s_servg_get_id(serv), s_string_data_p(fname));
		return;
	}

	struct s_file_meta_data ** pmd = s_hash_get_str(mserv->file_metadata, fname);
	if(!pmd) {
		s_log("[Error] rpc_send_md_back, from:%d-%d, file:%s, no file-meta-data here!", s_servg_get_type(serv),
				s_servg_get_id(serv), s_string_data_p(fname));

		icreate_fail_to_client(core, creating->req_id, creating->client_id);

		s_string_drop(fname);
		s_string_drop(creating->fname);
		s_hash_del_str(mserv->file_creating, fname);
		return;
	}

	struct s_file_meta_data * md = *pmd;

	creating->ncheck--;
	if(creating->ncheck == 0) {
		// create complete ! send ok to client
		icreate_suc_to_client(core, creating, md);

		s_string_drop(creating->fname);
		s_hash_del_str(mserv->file_creating, fname);
	}

	s_string_drop(fname);
}

void s_core_mserv_create_check_auth(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	struct s_string * fname;

	int ret = 0;

	if(s_packet_read_string(pkt, &fname) < 0) {
		ret = 0;
	} else {
		ret = icheck_can_create(core, s_servg_get_id(serv), fname);
	}

	struct s_packet * pkt_ret = s_packet_create(4);
	s_packet_write_int(pkt_ret, ret);
	s_servg_rpc_ret(serv, s_packet_get_req(pkt), pkt_ret);
	s_packet_drop(pkt_ret);

	return;
}

void s_core_mserv_create_metadata(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	struct s_file_meta_data * fmd = s_core_pkt_to_fmd(core, pkt);
	int ret = 1;
	if(!fmd) {
		s_log("[Error] other mserv create meta-data, read from packet error!");
		ret = 0;
		goto label_rpc_ret;
	}

	s_log("[LOG] other mserv create meta-data, file:%s", s_string_data_p(fmd->fname));

	struct s_mserver * mserv = s_core_mserv(core);

	struct s_file_meta_data ** pp = s_hash_set_str(mserv->file_metadata, fmd->fname);
	*pp = fmd;

label_rpc_ret:
	{
		struct s_packet * pkt_ret = s_packet_create(4);
		s_packet_write_int(pkt_ret, ret);
		s_servg_rpc_ret(serv, s_packet_get_req(pkt), pkt_ret);
		s_packet_drop(pkt_ret);
	}
}

void s_core_mserv_create_meta_meta_data(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	struct s_file_meta_meta_data * fmmd = s_core_pkt_to_fmmd(core, pkt);
	if(!fmmd) {
		s_log("[Error] other mserv create meta-meta-data, read from packet error!");
		return;
	}
	s_log("[LOG] other mserv(%d-%d) create meta-meta-data(file:%s)", s_servg_get_type(serv), s_servg_get_id(serv),
			s_string_data_p(fmmd->fname));
	s_log("[LOG] nmserv:%d", fmmd->nmserv);
	int i;
	for(i = 0; i < fmmd->nmserv; ++i) {
		s_log("[LOG] :%d", fmmd->mservs[i]);
	}

	struct s_mserver * mserv = s_core_mserv(core);
	struct s_file_meta_meta_data ** pp = s_hash_set_str(mserv->file_meta_metadata, fmmd->fname);
	*pp = fmmd;
}


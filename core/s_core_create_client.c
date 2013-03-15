#include "s_core.h"
#include "s_core_create.h"
#include "s_core_create_pkt.h"

#define error_out()	goto label_error

int s_core_create_file(struct s_core * core, struct s_core_metadata_param * param)
{
	int error = S_CORE_ERR_NOERR;
	char * errmsg = "";

	struct s_server_group * servg = core->servg;

	/* 1. get min-delay meta-server */
	struct s_server * mserv = s_servg_get_min_delay_serv(servg, S_SERV_TYPE_M);
	if(!mserv) {
		error = S_CORE_ERR_INTERNAL;
		errmsg = "no meta-server connected!";

		error_out();
	}

	struct s_conn * conn = s_servg_get_conn(mserv);
	if(!conn) {
		error = S_CORE_ERR_INTERNAL;
		errmsg = "meta-serv has no connection!";

		error_out();
	}

	struct s_serv_d * d = s_servg_get_udata(mserv);
	if(!d || !d->req_hash) {
		error = S_CORE_ERR_INTERNAL;
		errmsg = "no serv_d or no serv_d->req_hash in mserv!";
		
		error_out();
	}


	/* 2. check param */
	if(!param->filename) {
		error = S_CORE_ERR_BAD_PARAM;
		errmsg = "param : no filename provided";

		error_out();
	}
	if(!param->callback) {
		error = S_CORE_ERR_BAD_PARAM;
		errmsg = "param : no callback provided";

		error_out();
	}

	/* 3. create packet */
	struct s_packet * pkt = s_core_create_pkt_create(core, param);
	if(!pkt) {
		error = S_CORE_ERR_INTERNAL;
		errmsg = "no mem for create_pkt!";

		error_out();
	}

	/* 4. save rpc */
	unsigned int req_id;
	if(s_packet_read_req(pkt, &req_id) < 0) {
		error = S_CORE_ERR_INTERNAL;
		errmsg = "req_id create error!";
		
		error_out();
	}

	struct s_core_metadata_param * param_saved = s_core_metadata_param_create();
	memcpy(param_saved, param, sizeof(struct s_core_metadata_param));

	void ** pp = s_hash_set_num(d->req_hash, req_id);
	if(!pp) {
		error = S_CORE_ERR_INTERNAL;
		errmsg = "no mem for req_hash.set!";

		error_out();
	}
	*pp = param_saved;

	/* 5. send packet */
	s_net_send(conn, pkt);
	s_packet_drop(pkt);

	return 0;

label_error:
	{
		s_log("[Error] %s", errmsg);

		param->error = error;
		memcpy(param->errmsg, errmsg, strlen(errmsg) + 1);
	}

	return -1;
}

void s_core_client_create_back(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	s_used(core);

	int result;
	s_packet_read(pkt, &result, int, label_error);

	s_log("create back:%d", result);

	return;

label_error:
	return;
}


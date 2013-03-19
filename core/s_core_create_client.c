#include "s_core.h"
#include "s_core_create.h"
#include "s_core_create_pkt.h"

#define error_out()	goto label_error

static void s_core_client_create_back(struct s_conn * conn, struct s_packet * pkt, void * ud);

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

	/* 2. check param */
	if(!param->fname) {
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

	/* 4. save rpc param */
	struct s_core_metadata_param * param_saved = s_core_metadata_param_create(core);
	memcpy(param_saved, param, sizeof(struct s_core_metadata_param));

	/* 5. call rpc / send packet */
	s_net_rpc_call(conn, pkt, param_saved, &s_core_client_create_back);
	s_packet_drop(pkt);

	return 0;

label_error:
	{
		s_log("[Error] create file: %s", errmsg);

		param->error = error;
		memcpy(param->errmsg, errmsg, strlen(errmsg) + 1);
	}

	return -1;
}

static void s_core_client_create_back(struct s_conn * conn, struct s_packet * pkt, void * ud)
{
	struct s_server * serv = s_servg_get_serv_from_conn(conn);
	s_used(serv);

	struct s_core_metadata_param * param = (struct s_core_metadata_param *)ud;
	s_used(param);

	struct s_core * core = param->core;
	s_used(core);

	int result;
	s_packet_read(pkt, &result, int, label_error);

	s_log("create back:%d, %s, %u-%u", result, s_string_data_p(param->fname), param->size.high, param->size.low);

	param->callback(param);

	s_core_metadata_param_destroy( param );

	return;

label_error:
	return;
}


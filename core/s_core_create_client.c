#include "s_core.h"
#include "s_core_create.h"
#include "s_core_pkt.h"

#define error_out()	goto label_error

static void s_core_client_create_back(struct s_server * serv, struct s_packet * pkt, void * ud);

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
	struct s_packet * pkt = s_core_pkt_from_mdp(core, param);
	if(!pkt) {
		error = S_CORE_ERR_INTERNAL;
		errmsg = "no mem for create_pkt!";

		error_out();
	}

	/* 4. save rpc param */
	struct s_core_metadata_param * param_saved = s_core_metadata_param_create(core);
	memcpy(param_saved, param, sizeof(struct s_core_metadata_param));

	/* 5. call rpc / send packet */
	s_servg_rpc_call(mserv, pkt, param_saved, &s_core_client_create_back, -1);
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

static void s_core_client_create_back(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core_metadata_param * param = (struct s_core_metadata_param *)ud;
	s_used(param);

	struct s_core * core = param->core;
	s_used(core);

	param->error = S_CORE_ERR_NOERR;
	const char * errmsg = NULL;

	if(!pkt) {
		s_log("[Warning] create file:%s, timeout or conn broken!", s_string_data_p(param->fname));
		param->error = S_CORE_ERR_INTERNAL;
		errmsg = "timeout or conn-broken";
	} else {
		int result;
		if(s_packet_read_int(pkt, &result) < 0) {
			param->error = S_CORE_ERR_INTERNAL;
			errmsg = "rpc return error";
		}
		if(result <= 0) {
			param->error = S_CORE_ERR_INTERNAL;
			errmsg = "create failed";
		}
	}

	if(errmsg) {
		s_log("[lOG] create(%s) error:%s", s_string_data_p(param->fname), errmsg);
		memcpy(param->errmsg, errmsg, strlen(errmsg) + 1);
	} else {
		s_log("[LOG] create(%s) ok", s_string_data_p(param->fname));
	}

	param->callback(param);

	s_core_metadata_param_destroy( param );

	return;
}


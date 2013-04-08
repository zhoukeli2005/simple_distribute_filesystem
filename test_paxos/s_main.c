#include "s_server.h"

#include <signal.h>

int main(int argc, char * argv[])
{
	/* 1 -- ingore signals : SIGPIPE -- */
	struct sigaction act = {
		.sa_handler = SIG_IGN,
		.sa_flags = 0
	};
	sigemptyset(&act.sa_mask);

	if(sigaction(SIGPIPE, &act, NULL) < 0) {
		s_log("[Error] ignore SIGPIPE error!");
		perror("SIGPIPE");
		return 0;
	}

	/* 2 -- read config.conf and init s_mserver -- */
	struct s_config * config = s_config_create("config.conf");
	if(!config) {
		s_log("[Error] open config error!");
		return 0;
	}

	/* 3 -- parse serv_id from program name -- */
	int id = s_util_get_serv_id(argv[0], '_');

	int type = -1;

	if(strstr(argv[0], "mserv")) {
		// it is mserver
		type = S_SERV_TYPE_M;
	}
	if(strstr(argv[0], "dserv")) {
		// it is a dserv
		if(type != -1) {
			s_log("[Error] the program name(%s) **must not** contain both 'mserv' and 'dserv'!", argv[0]);
			return 0;
		}
		type = S_SERV_TYPE_D;
	}

	if(type == -1) {
		// it is a client
		type = S_SERV_TYPE_C;
	}

	if(type != S_SERV_TYPE_C && id <= 0) {
		s_log("[Error] miss id!");
		return 0;
	}

	/* 3.2 -- parse id we will connect -- */
	g_ud.serv_id = -1;
	if(argc > 1) {
		g_ud.serv_id = atoi(argv[1]);
		s_log("[LOG] will connect to %d", g_ud.serv_id);
	}

	/* 4 -- create main data-structure : s_core -- */

	struct s_core_create_param create_param = {
		.type = type,
		.id = id,
		.config = config
	};

	/* 4.1 -- if it's mserv, do mserv init -- */
	if(type == S_SERV_TYPE_M) {
		create_param.all_established = s_ud_client_init;
		create_param.update = s_ud_client_update;
	}

	struct s_core * core = s_core_create(&create_param);

	if(!core) {
		s_log("s_core create error!");
		return 0;
	}

	s_log("core create ok, id:%d", id);

	/* 4.2 -- connect to mserv -- */
	if(g_ud.serv_id <= 0) {
		g_ud.serv_id = 1;
	}
	if(g_ud.serv_id > 0) {
		g_ud.serv = s_servg_connect(core->servg, S_SERV_TYPE_M, g_ud.serv_id,
				"127.0.0.1", S_SERV_M_DEFAULT_PORT + g_ud.serv_id);
	}

	/* 5 -- do main process -- */
	while(1) {
		/* 1. do servg stuff */
		if(s_core_poll(core, 10) < 0) {
			s_log("[Error] s_core_poll error!");
			break;
		}
	}
	return 0;
}


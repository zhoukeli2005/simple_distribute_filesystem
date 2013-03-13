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
	if(id <= 0) {
		s_log("[Error] miss id!");
		return 0;
	}

	/* 4 -- create main data-structure : s_core -- */

	struct s_core * core = s_core_create(S_SERV_TYPE_M, id, config);

	if(!core) {
		s_log("s_core create error!");
		return 0;
	}

	s_log("core create ok, id:%d", id);

	/* 4 -- do main process -- */
	while(1) {
		/* 1. do servg stuff */
		if(s_core_poll(core, 10) < 0) {
			s_log("[Error] s_core_poll error!");
			break;
		}
	}
	return 0;
}


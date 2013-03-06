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
		s_log("ignore SIGPIPE error!");
		perror("SIGPIPE");
		return 0;
	}

	/* 2 -- read config.conf and init s_mserver -- */
	struct s_config * config = s_config_create("config.conf");
	if(!config) {
		s_log("open config error!");
		return 0;
	}

	/* 3 -- create main data-structure : s_mserver -- */

	struct s_mserver * mserv = s_mserver_create(argc, argv, config);

	if(!mserv) {
		s_log("mserv create error!");
		return 0;
	}

	s_log("mserv create ok, id:%d", mserv->id);

	/* 4 -- do main process -- */
	while(1) {
		/* 1. do servg stuff */
		if(s_servg_poll(mserv->servg, 10) < 0) {
			s_log("[Error] servg_poll error!");
			break;
		}
	}
	return 0;
}


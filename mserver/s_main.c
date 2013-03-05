#include "s_server.h"
#include "s_check_conn.h"

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

	/* 2 -- create main data-structure : s_mserver -- */

	struct s_mserver * mserv = s_mserver_create(argc, argv);

	if(!mserv) {
		s_log("mserv create error!");
		return 0;
	}

	s_log("mserv create ok, id:%d", mserv->id);

	/* 3 -- read config.conf and init s_mserver -- */
	struct s_config * config = s_config_create("config.conf");
	if(!config) {
		s_log("open config error!");
		return 0;
	}

	if(s_mserver_init_config(mserv, config) < 0) {
		s_log("init config error!");
		return 0;
	}

	/* 4 -- do main process -- */
	while(1) {
		/* 1. check net events */
		if(s_net_poll(mserv->net, 10) < 0) {
			s_log("poll error!");
			break;
		}

		/* 2. check connections */
		if(s_check_conn(mserv) < 0) {
			s_log("check conn error!");
			break;
		}

	}
	return 0;
}


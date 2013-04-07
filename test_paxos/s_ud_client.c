#include "s_server.h"

static struct timeval g_Time = {0};

void * s_ud_client_init(struct s_core * core)
{
	return NULL;
}

void s_ud_client_update(struct s_core * core, void * ud)
{
	if(!g_Time.tv_sec) {
		gettimeofday(&g_Time, NULL);
		return;
	}

	struct timeval now, sub;
	gettimeofday(&now, NULL);

	timersub(&now, &g_Time, &sub);
	if(sub.tv_sec > 5) {

		static int version = 1;

		s_log("[LOG] start paxos...");

		struct s_string * topic = s_string_create_format("version %d", version++);
		s_paxos_start(core, topic, 1);

		g_Time = now;
	}
}


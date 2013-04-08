#include "s_server.h"

static struct timeval g_Time = {0};
static struct timeval g_LearnTime = {0};

struct s_ud g_ud;

void * s_ud_client_init(struct s_core * core)
{
	return NULL;
}

void s_ud_client_update(struct s_core * core, void * ud)
{
	if(!g_Time.tv_sec) {
		gettimeofday(&g_Time, NULL);
		g_Time.tv_sec -= 58;
		return;
	}

	struct timeval now, sub;
	gettimeofday(&now, NULL);

	timersub(&now, &g_LearnTime, &sub);

	if(sub.tv_sec > 5  && core->paxos->version.version > 0) {
		g_LearnTime = now;

		s_paxos_learn_all(core);
	}

	timersub(&now, &g_Time, &sub);

	if(!s_paxos_is_in(core->paxos, core->id)) {
		static int sent = 0;
		if(sub.tv_sec > 2 && !sent) {
			struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_PAXOS_ADD_ME, 0);
			s_servg_send(g_ud.serv, pkt);
			s_packet_drop(pkt);

			g_Time = now;
			sent = 1;
		}
		return;
	}

	if(core->id == 1 && sub.tv_sec > 60 && s_paxos_is_in(core->paxos, core->id)) {

		g_Time = now;

		struct s_string * topic = s_string_create("x");

		static int value = 3;

		s_paxos_start(core, topic, value++, S_PAXOS_PROPOSAL_NORMAL, 0);
	}
}


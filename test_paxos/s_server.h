#ifndef s_server_h_
#define s_server_h_

#include <s_net.h>
#include <s_core.h>
#include <s_core_paxos.h>
#include <s_misc.h>
#include <s_packet.h>
#include <s_list.h>
#include <s_array.h>
#include <s_config.h>
#include <s_common.h>
#include <s_thread.h>
#include <s_ipc.h>
#include <s_server_group.h>

struct s_ud {
	int serv_id;
	struct s_server * serv;
};

extern struct s_ud g_ud;


void *
s_ud_client_init(struct s_core * core);
void
s_ud_client_update(struct s_core * core, void * ud);

#endif


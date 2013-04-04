#ifndef s_core_glock_h_
#define s_core_glock_h_

#include "s_core.h"

void s_core_mserv_glock(struct s_server * serv, struct s_packet * pkt, void * ud);

void s_core_mserv_glock_unlock(struct s_server * serv, struct s_packet * pkt, void * ud);

#endif


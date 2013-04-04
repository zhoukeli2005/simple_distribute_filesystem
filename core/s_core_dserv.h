#ifndef s_core_dserv_h_
#define s_core_dserv_h_

#include "s_core.h"

void s_core_dserv_push_data(struct s_server * serv, struct s_packet * pkt, void * ud);

void s_core_dserv_write(struct s_server * serv, struct s_packet * pkt, void * ud);

void s_core_dserv_lock(struct s_server * serv, struct s_packet * pkt, void * ud);

void s_core_dserv_lock_unlock(struct s_server * serv, struct s_packet * pkt, void * ud);

#endif


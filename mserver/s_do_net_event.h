#ifndef s_do_net_event_h_
#define s_do_net_event_h_

#include "s_server.h"

void
s_do_net_event_m(struct s_server * serv, struct s_packet * pkt, void * ud);

void
s_do_net_event_d(struct s_server * serv, struct s_packet * pkt, void * ud);

void
s_do_net_event_c(struct s_server * serv, struct s_packet * pkt, void * ud);

#endif


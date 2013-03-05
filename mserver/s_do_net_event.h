#ifndef s_do_net_event_h_
#define s_do_net_event_h_

#include "s_server.h"

void
s_do_net_event(struct s_conn * conn, struct s_packet * pkt, void * ud);

void
s_do_net_event_m(struct s_mserver * mserv, struct s_conn * conn, struct s_packet * pkt);

void
s_do_net_event_d(struct s_mserver * mserv, struct s_conn * conn, struct s_packet * pkt);

void
s_do_net_event_c(struct s_mserver * mserv, struct s_conn * conn, struct s_packet * pkt);

#endif


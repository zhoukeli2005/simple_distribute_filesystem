#include "s_do_net_event.h"

void s_do_net_event(struct s_conn * conn, struct s_packet * pkt)
{
	if(pkt == S_NET_CONN_CLOSED) {
		s_log("a connection is closed(%s:%d)", s_net_ip(conn), s_net_port(conn));
		return;
	}

	if(pkt == S_NET_CONN_ACCEPT) {
		s_log("a connection comes(%s:%d)", s_net_ip(conn), s_net_port(conn));
		return;
	}

	if(pkt == S_NET_CONN_CLOSING) {
		s_log("a connection closing(%s:%d)", s_net_ip(conn), s_net_port(conn));
		return;
	}

	s_log("receive a packet, size:%d", s_packet_size(pkt));

	return;
}

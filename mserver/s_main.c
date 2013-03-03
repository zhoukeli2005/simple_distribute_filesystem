#include <s_common.h>
#include <s_net.h>
#include <s_thread.h>
#include <s_packet.h>
#include <s_config.h>

static void do_conn_event(struct s_conn * conn, struct s_packet * pkt);

int main(int argc, char * argv[])
{
	struct s_config * config = s_config_create("config.conf");
	if(!config) {
		s_log("open config error!");
		return 0;
	}
	s_config_iter_begin(config);
	struct s_string * region_name = s_config_iter_next(config);
	while(region_name) {
		s_log("region:%s", s_string_data_p(region_name));
		region_name = s_config_iter_next(config);
	}
		

	struct s_net * net = s_net_create(2046, &do_conn_event);

	while(1) {
		if(s_net_poll(net, 10) < 0) {
			s_log("poll error!");
			break;
		}
	}
	return 0;
}

static void do_conn_event(struct s_conn * conn, struct s_packet * pkt)
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
}


#include <s_common.h>
#include <s_net.h>
#include <s_thread.h>
#include <s_packet.h>

static void do_conn_event(struct s_conn * conn, struct s_packet * pkt);

int main(int argc, char * argv[])
{
	struct s_net * net = s_net_create(0, &do_conn_event);

	struct s_conn * conn = s_net_connect(net, "127.0.0.1", 2046);

	struct s_packet * pkt = s_packet_create(1024);
	s_net_send(conn, pkt);

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
}

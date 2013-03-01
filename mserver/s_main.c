#include <s_common.h>
#include <s_net.h>
#include <s_thread.h>
#include <s_packet.h>

static void do_conn_event(struct s_conn * conn, struct s_packet * pkt);

int main(int argc, char * argv[])
{
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
}

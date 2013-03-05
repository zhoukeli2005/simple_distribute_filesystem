#include "s_do_net_event.h"

static void ihandle_identify_back(struct s_mserver * mserv, struct s_server * serv, struct s_packet * pkt)
{
	int ret;
	s_packet_read(pkt, &ret, int, label_read_error);

	if(ret == 1) {
		s_log("identify ok!");

		// move to `wait_for_ping list`
		s_list_del(&serv->list);
		s_list_insert_tail(&mserv->wait_for_ping_list, &serv->list);

		// set flag
		serv->flags = S_SERV_FLAG_ESTABLISH | S_SERV_FLAG_VALID;

		return;
	}


	s_log("identify error!");

label_read_error:

	s_log("read data error!");

	s_mserver_clear_server(mserv, serv);
	
	return;
}

static void ihandle_ping(struct s_mserver * mserv, struct s_server * serv, struct s_packet * pkt)
{
	s_ipc_pkt_pong(pkt);

	s_net_send(serv->conn, pkt);

}

static void ihandle_pong(struct s_mserver * mserv, struct s_server * serv, struct s_packet * pkt)
{
	unsigned int sec, usec;
	s_packet_read(pkt, &sec, uint, label_read_error);
	s_packet_read(pkt, &usec, uint, label_read_error);

	s_log("handle pong, sec:%u, usec:%u", (unsigned int)sec, (unsigned int)usec);
	struct timeval tv_now, tv_sub;
	gettimeofday(&tv_now, NULL);

	struct timeval tv_sent = {
		.tv_sec = sec,
		.tv_usec = usec
	};
	timersub(&tv_now, &tv_sent, &tv_sub);

	s_log("round-time:%u-%u", (unsigned int)tv_sub.tv_sec, (unsigned int)tv_sub.tv_usec);

	serv->tv_receive_pong = tv_now;

	// move to `wait for ping list`
	s_list_del(&serv->list);
	s_list_insert_tail(&mserv->wait_for_ping_list, &serv->list);

	return;

label_read_error:

	return;
}

void s_do_net_event_m(struct s_mserver * mserv, struct s_conn * conn, struct s_packet * pkt)
{
	struct s_server * serv = s_net_get_udata(conn);

	int type;
	if(s_packet_read_int(pkt, &type) < 0) {
		s_log("read type error!");
		return;
	}

	switch(type) {
		case S_PKT_TYPE_IDENTIFY_BACK:
			ihandle_identify_back(mserv, serv, pkt);
			break;

		case S_PKT_TYPE_PING:
			ihandle_ping(mserv, serv, pkt);
			break;

		case S_PKT_TYPE_PONG:
			ihandle_pong(mserv, serv, pkt);
			break;

		default:
			s_log("un-expected packet, type:%d", type);
			break;
	}
}


#include "s_check_conn.h"
#include "s_server.h"

int s_check_conn(struct s_mserver * mserv)
{
	struct s_list * p, * tmp;

	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);

	// 1. wait for connection

	s_list_foreach_safe(p, tmp, &mserv->wait_for_conn_list) {
		struct s_server * serv = s_list_entry(p, struct s_server, list);
		assert(!serv->conn && (serv->flags & S_SERV_FLAG_VALID));

		struct timeval tv_sub;
		timersub(&tv_now, &serv->tv_connect, &tv_sub);
		if(timerisset(&serv->tv_connect) && tv_sub.tv_sec <= mserv->sec_for_reconnect) {
			break;
		}

		serv->tv_connect = tv_now;

		serv->conn = s_net_connect(mserv->net, s_string_data_p(serv->ip), serv->port);
		if(!serv->conn) {
			continue;
		}

		s_log("connected to (%s:%d)", s_string_data_p(serv->ip), serv->port);

		// add udata
		s_net_set_udata(serv->conn, serv);

		// delete from wait for conn list
		s_list_del(p);

		// insert into wait for identify list
		s_list_insert_tail(&mserv->wait_for_identify_list, p);

		// set timeval
		serv->tv_send_identify = tv_now;

		// send identification
		struct s_packet * pkt;
		s_ipc_pkt_identify(pkt, mserv->id, S_SERV_TYPE_M, mserv->ipc_pwd);
		s_net_send(serv->conn, pkt);
		s_packet_drop(pkt);

		// set flags
		serv->flags = S_SERV_FLAG_IDENTIFY | S_SERV_FLAG_VALID;
	}


	// 2. wait for identify

	s_list_foreach_safe(p, tmp, &mserv->wait_for_identify_list) {
		struct s_server * serv = s_list_entry(p, struct s_server, list);
		struct timeval tv_sub;
		timersub(&tv_now, &serv->tv_send_identify, &tv_sub);
		if(tv_sub.tv_sec <= mserv->sec_for_timeover) {
			break;
		}

		// time over, clear server
		s_mserver_clear_server(mserv, serv);
	}


	// 3. wait for ping
	s_list_foreach_safe(p, tmp, &mserv->wait_for_ping_list) {
		struct s_server * serv = s_list_entry(p, struct s_server, list);
		struct timeval tv_sub;
		timersub(&tv_now, &serv->tv_receive_pong, &tv_sub);
		if(tv_sub.tv_sec <= mserv->sec_for_pingpong) {
			break;
		}

		// ping other end
		s_list_del(p);
		s_list_insert_tail(&mserv->wait_for_pong_list, p);

		// set timeval
		serv->tv_send_ping = tv_now;

		// send packet
		struct s_packet * pkt;
		s_ipc_pkt_ping(pkt, tv_now.tv_sec, tv_now.tv_usec);
		s_net_send(serv->conn, pkt);
		s_packet_drop(pkt);

		// set flag
		serv->flags |= S_SERV_FLAG_SEND_PING;
	}

	// 4. wait for pong
	s_list_foreach_safe(p, tmp, &mserv->wait_for_pong_list) {
		struct s_server * serv = s_list_entry(p, struct s_server, list);
		struct timeval tv_sub;
		timersub(&tv_now, &serv->tv_send_ping, &tv_sub);
		if(tv_sub.tv_sec <= mserv->sec_for_timeover) {
			break;
		}

		// time over
		s_log("wait for pong time over, (%s:%d), re-connect!", s_string_data_p(serv->ip), serv->port);

		s_mserver_clear_server(mserv, serv);
	}

	return 0;
}


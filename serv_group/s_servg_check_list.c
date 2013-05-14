#include <s_mem.h>
#include "s_servg.h"
#include "s_servg_pkt.h"

int s_servg_check_list(struct s_server_group * servg)
{
	struct s_list * p, * tmp;

	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);

	// 1. wait for connection

	s_list_foreach_safe(p, tmp, &servg->list_wait_for_conn) {
		struct s_server * serv = s_list_entry(p, struct s_server, list);
		assert(!serv->conn);
		assert(serv->flags & S_SERV_FLAG_IN_CONFIG);

		struct timeval tv_sub;
		timersub(&tv_now, &serv->tv_connect, &tv_sub);
		if(timerisset(&serv->tv_connect) && tv_sub.tv_sec <= servg->sec_for_reconnect) {
			break;
		}

		serv->tv_connect = tv_now;

		serv->conn = s_net_connect(servg->net, s_string_data_p(serv->ip), serv->port);
		if(!serv->conn) {
			continue;
		}

		s_log("[LOG]connected to (%s:%d)", s_string_data_p(serv->ip), serv->port);

		// add udata
		s_net_set_udata(serv->conn, serv);

		// delete from wait for conn list
		s_list_del(p);

		// insert into wait for identify list
		s_list_insert_tail(&servg->list_wait_for_identify, p);

		// set timeval
		serv->tv_send_identify = tv_now;

		// send identification
		struct s_packet * pkt = s_servg_pkt_identify(servg->type, servg->id, servg->ipc_pwd);
		s_net_send(serv->conn, pkt);
		s_packet_drop(pkt);
	}

/*

	// 2. wait for identify

	s_list_foreach_safe(p, tmp, &servg->list_wait_for_identify) {
		struct s_server * serv = s_list_entry(p, struct s_server, list);
		struct timeval tv_sub;
		timersub(&tv_now, &serv->tv_send_identify, &tv_sub);
		if(tv_sub.tv_sec <= servg->sec_for_timeover) {
			break;
		}

		// time over, clear server
		s_servg_reset_serv(servg, serv);
	}


	// 3. wait for ping
	s_list_foreach_safe(p, tmp, &servg->list_wait_for_ping) {
		struct s_server * serv = s_list_entry(p, struct s_server, list);
		struct timeval tv_sub;
		timersub(&tv_now, &serv->tv_receive_pong, &tv_sub);
		if(tv_sub.tv_sec <= servg->sec_for_pingpong) {
			break;
		}

		// ping other end
		s_list_del(p);
		s_list_insert_tail(&servg->list_wait_for_pong, p);

		// set timeval
		serv->tv_send_ping = tv_now;

		// send packet
		struct s_packet * pkt = s_servg_pkt_ping(tv_now.tv_sec, tv_now.tv_usec);
		s_net_send(serv->conn, pkt);
		s_packet_drop(pkt);
	}

	// 4. wait for pong
	s_list_foreach_safe(p, tmp, &servg->list_wait_for_pong) {
		struct s_server * serv = s_list_entry(p, struct s_server, list);
		struct timeval tv_sub;
		timersub(&tv_now, &serv->tv_send_ping, &tv_sub);
		if(tv_sub.tv_sec <= servg->sec_for_timeover) {
			break;
		}

		struct s_string * ip = serv->ip;
		int port = serv->port;

		// time over
		if(ip) {
			s_log("[LOG]wait for pong time over, (%s:%d), re-connect!", s_string_data_p(ip), port);
		}

		s_servg_reset_serv(servg, serv);
	}*/

	return 0;
}


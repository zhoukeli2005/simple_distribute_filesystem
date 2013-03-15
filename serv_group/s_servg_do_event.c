#include "s_servg.h"
#include "s_servg_pkt.h"

static void iwhen_conn_closed(struct s_server_group * servg, struct s_conn * conn)
{
	struct s_server * serv = s_net_get_udata(conn);
	if(!serv) {
		return;
	}

	s_log("[LOG] conn closed! type:%d, id:%d", serv->type, serv->id);
	serv->conn = NULL;

	if(servg->callback.serv_closed) {
		servg->callback.serv_closed(serv, servg->callback.udata);
	}

	s_servg_reset_serv(servg, serv);
}

void ihandle_identify(struct s_server_group * servg, struct s_conn * conn, struct s_packet * pkt)
{
	unsigned int fun;
	s_packet_read(pkt, &fun, fun, label_read_error);

	if(fun != S_PKT_TYPE_IDENTIFY) {
		s_log("[Error]expect identify, got:%u", fun);
		s_net_close(conn);
		return;
	}

	unsigned int pwd;
	s_packet_read(pkt, &pwd, uint, label_read_error);
	if(pwd != servg->ipc_pwd) {
		s_log("[Error]identify:pwd(%x) != servg->ipc_pwd(%x)", pwd, servg->ipc_pwd);
		goto label_auth_error;
	}

	int type;
	s_packet_read(pkt, &type, int, label_read_error);

	int id;
	s_packet_read(pkt, &id, int, label_read_error);

	struct s_server * serv = s_servg_get_serv(servg, type, id);
	if(!serv) {
		s_log("[LOG] a server comes, not in config.conf");
		serv = s_servg_create_serv(servg);
		if(!serv) {
			goto label_auth_error;
		}
		serv->id = id;
		serv->type = type;
	}

	if(serv->flags & S_SERV_FLAG_ESTABLISHED) {
		s_log("serv(id:%d, type:%d) has already Established!", id, type);
		goto label_auth_error;
	}

	s_log("serv auth ok:<id:%d><type:%d>", id, type);

	// add udata
	s_net_set_udata(conn, serv);
	serv->conn = conn;

	// set flags
	serv->flags |= S_SERV_FLAG_ESTABLISHED;

	// set time
	gettimeofday(&serv->tv_connect, NULL);
	serv->tv_established = serv->tv_connect;

	// add to `wait for ping list`
	s_list_del(&serv->list);
	s_list_insert_tail(&servg->list_wait_for_ping, &serv->list);

	// auth ok
	struct s_packet * tmp = s_servg_pkt_identify_back(1);
	s_net_send(conn, tmp);
	s_packet_drop(tmp);

	// callback
	if(servg->callback.serv_established) {
		servg->callback.serv_established(serv, servg->callback.udata);
	}

	return;

label_read_error:

	s_net_close(conn);
	return;

label_auth_error:
	{
		struct s_packet * tmp = s_servg_pkt_identify_back(0);
		s_net_send(conn, tmp);
		s_packet_drop(tmp);
	}
	return;
}

static void ihandle_identify_back(struct s_server_group * servg, struct s_server * serv, struct s_packet * pkt)
{
	int ret;
	s_packet_read(pkt, &ret, int, label_read_error);

	if(ret == 1) {
		s_log("[LOG] identify ok!");

		// move to `wait_for_ping list`
		s_list_del(&serv->list);
		s_list_insert_tail(&servg->list_wait_for_ping, &serv->list);

		// set flag
		serv->flags |= S_SERV_FLAG_ESTABLISHED;

		// callback
		if(servg->callback.serv_established) {
			servg->callback.serv_established(serv, servg->callback.udata);
		}

		return;
	}   

	s_log("[LOG]identify error!");

label_read_error:

	s_servg_reset_serv(servg, serv);

	return;
}

static void ihandle_ping(struct s_server_group * servg, struct s_server * serv, struct s_packet * pkt)
{
	s_servg_pkt_pong(pkt);

	s_net_send(serv->conn, pkt);
}

static void ihandle_pong(struct s_server_group * servg, struct s_server * serv, struct s_packet * pkt)
{
	unsigned int sec, usec;
	s_packet_read(pkt, &sec, uint, label_read_error);
	s_packet_read(pkt, &usec, uint, label_read_error);

	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);

	struct timeval tv_sent = {
		.tv_sec = sec,
		.tv_usec = usec
	};
	timersub(&tv_now, &tv_sent, &serv->tv_delay);

	struct s_server * min_delay_serv = servg->min_delay_serv[serv->type];
	if(!min_delay_serv ||
			(min_delay_serv != serv &&
			 timercmp(&min_delay_serv->tv_delay, &serv->tv_delay, >)
			)
	) {
		servg->min_delay_serv[serv->type] = serv;
	}

	serv->tv_receive_pong = tv_now;

	s_log("roundup-time: %u, %u", (unsigned int)(serv->tv_delay.tv_sec), (unsigned int)(serv->tv_delay.tv_usec));

	// move to `wait for ping list`
	s_list_del(&serv->list);
	s_list_insert_tail(&servg->list_wait_for_ping, &serv->list);

	return;

label_read_error:

	return;
}

void s_servg_do_event(struct s_conn * conn, struct s_packet * pkt, void * udata)
{
	struct s_server_group * servg = (struct s_server_group *)udata;

	if(pkt == S_NET_CONN_CLOSED) {
		iwhen_conn_closed(servg, conn);
		return;
	}

	if(pkt == S_NET_CONN_ACCEPT) {
		s_log("[LOG]a connection comes(%s:%d)", s_net_ip(conn), s_net_port(conn));
		return;
	}

	if(pkt == S_NET_CONN_CLOSING) {
		iwhen_conn_closed(servg, conn);
		return;
	}

	struct s_server * serv = s_net_get_udata(conn);
	if(!serv) {
		// must be identify
		ihandle_identify(servg, conn, pkt);
		return;
	}

	unsigned int fun;
	if(s_packet_read_fun(pkt, &fun) < 0) {
		s_log("[Error] packet must start with Function!");
		return;
	}

	switch(fun) {
		case S_PKT_TYPE_IDENTIFY_BACK:
			ihandle_identify_back(servg, serv, pkt);
			return;

		case S_PKT_TYPE_PING:
			ihandle_ping(servg, serv, pkt);
			return;

		case S_PKT_TYPE_PONG:
			ihandle_pong(servg, serv, pkt);
			return;

		default:
			break;
	}

	int type = serv->type;

	if(servg->callback.handle_msg[type][fun]) {
		servg->callback.handle_msg[type][fun](serv, pkt, servg->callback.udata);
	}

	return;
}


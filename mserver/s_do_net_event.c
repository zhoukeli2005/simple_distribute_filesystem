#include "s_do_net_event.h"

static void iwhen_conn_closed(struct s_mserver * mserv, struct s_conn * conn)
{
	struct s_serv_client_header * h = s_net_get_udata(conn);
	if(!h) {
		return;
	}

	if(h->type == S_SERV_TYPE_C) {
		s_log("client conn closed!(%s:%d)", s_net_ip(conn), s_net_port(conn));
		struct s_client * client = (struct s_client *)h;
		(void)(client);
		// XXX
		return;
	}

	if(h->type == S_SERV_TYPE_M) {
		s_log("meta-server conn closed!(%s:%d)", s_net_ip(conn), s_net_port(conn));
		struct s_server * serv = (struct s_server *)h;

		serv->conn = NULL;

		s_mserver_clear_server(mserv, serv);

		return;
	}

	assert(h->type == S_SERV_TYPE_D);
	if(h->type == S_SERV_TYPE_D) {
		s_log("data-server conn closed!(%s:%d)", s_net_ip(conn), s_net_port(conn));
		// XXX

		struct s_server * serv = (struct s_server *)h;

		serv->conn = NULL;

		s_mserver_clear_server(mserv, serv);

		return;
	}
}

void ihandle_identify(struct s_mserver * mserv, struct s_conn * conn, struct s_packet * pkt)
{
	int tt;
	s_packet_read(pkt, &tt, int, label_read_error);

	if(tt != S_PKT_TYPE_IDENTIFY) {
		s_log("expect identify, got:%d", tt);
		s_net_close(conn);
		return;
	}

	unsigned int pwd;
	s_packet_read(pkt, &pwd, uint, label_read_error);
	if(pwd != mserv->ipc_pwd) {
		s_log("identify:pwd(%x) != mserv->ipc_pwd(%x)", pwd, mserv->ipc_pwd);
		goto label_auth_error;
	}

	int type;
	s_packet_read(pkt, &type, int, label_read_error);

	if(type == S_SERV_TYPE_C) {
		// new client
		s_log("new client");
		return;
	}

	int id;
	s_packet_read(pkt, &id, int, label_read_error);

	struct s_server * serv = s_mserver_get_serv(mserv, type, id);
	if(!serv) {
		s_log("serv(id:%d, type:%d) is not in config.conf!", id, type);
		goto label_auth_error;
	}

	if(serv->flags & S_SERV_FLAG_ESTABLISH) {
		s_log("serv(id:%d, type:%d) has already Established!", id, type);
		goto label_auth_error;
	}

	s_log("serv auth ok:<id:%d><type:%d>", id, type);

	// add udata
	s_net_set_udata(conn, serv);
	serv->conn = conn;

	// set flags
	serv->flags = S_SERV_FLAG_VALID | S_SERV_FLAG_ESTABLISH;

	// set time
	gettimeofday(&serv->tv_connect, NULL);
	serv->tv_established = serv->tv_connect;

	// add to `wait for ping list`
	s_list_del(&serv->list);
	s_list_insert_tail(&mserv->wait_for_ping_list, &serv->list);

	// auth ok
	struct s_packet * tmp;
	s_ipc_pkt_identify_back(tmp, 1);
	s_net_send(conn, tmp);
	s_packet_drop(tmp);

	return;

label_read_error:

	s_net_close(conn);
	return;

label_auth_error:
	{
		struct s_packet * tmp;
		s_ipc_pkt_identify_back(tmp, 0);
		s_net_send(conn, tmp);
		s_packet_drop(tmp);
	}
	return;
}

void s_do_net_event(struct s_conn * conn, struct s_packet * pkt, void * udata)
{
	struct s_mserver * mserv = (struct s_mserver *)udata;

	if(pkt == S_NET_CONN_CLOSED) {
		s_log("a connection is closed(%s:%d)", s_net_ip(conn), s_net_port(conn));
		iwhen_conn_closed(mserv, conn);
		return;
	}

	if(pkt == S_NET_CONN_ACCEPT) {
		s_log("a connection comes(%s:%d)", s_net_ip(conn), s_net_port(conn));
		return;
	}

	if(pkt == S_NET_CONN_CLOSING) {
		s_log("a connection closing(%s:%d)", s_net_ip(conn), s_net_port(conn));
		iwhen_conn_closed(mserv, conn);
		return;
	}

	s_log("receive a packet, size:%d", s_packet_size(pkt));

	struct s_serv_client_header * h = s_net_get_udata(conn);
	if(!h) {
		// must be identify
		ihandle_identify(mserv, conn, pkt);
		return;
	}

	if(h->type == S_SERV_TYPE_C) {
		// XXX handle client
		return;
	}

	if(h->type == S_SERV_TYPE_D) {
		// XXX handle data-serv
		return;
	}

	if(h->type == S_SERV_TYPE_M) {
		s_do_net_event_m(mserv, conn, pkt);
		return;
	}

	return;
}


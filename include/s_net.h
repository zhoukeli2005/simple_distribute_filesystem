#ifndef s_net_h_
#define s_net_h_

struct s_net;
struct s_conn;
struct s_packet;

#define S_NET_CONN_ACCEPT		(struct s_packet *)(0)
#define S_NET_CONN_CLOSED		(struct s_packet *)(1)
#define S_NET_CONN_CLOSING		(struct s_packet *)(2)

typedef void(*S_NET_CALLBACK)(struct s_conn * conn, struct s_packet * pkt);

/*
 *	create a net manager
 *
 *	@param listen_port : if listen_port > 0, net will run as a server, listenning at listen_port,
 *				waiting for connection coming
 *	@param callback:
 *			1. when a new connection comes, if will call :
 *				callback(conn, S_NET_CONN_ACCEPT)
 *
 *			2. when a new packet comes, if will call :
 *				 callback(conn, pkt)
 *
 *			3. when a connection is closed by the other end, if will call :
 *				callback(conn, S_NET_CONN_CLOSED)
 *
 *			4. when a connection is closed by called s_net_close(...), if will call : 
 *				callback(conn, S_NET_CONN_CLOSING)
 */
struct s_net *
s_net_create(int listen_port, S_NET_CALLBACK callback);


/*
 *	poll on all sockets : receive coming packets and send pedinning packets
 *
 *	@msec : wait for msec if no event comes
 */
int
s_net_poll(struct s_net * net, int msec);


/*
 *	connect to a server
 *
 *	@param ip :
 *	@param port : (host endian)
 */
struct s_conn *
s_net_connect(struct s_net * net, const char * ip, int port);


/*
 *	send a packet to a connection
 *
 */
void
s_net_send(struct s_conn * conn, struct s_packet * pkt);


/*
 *	close a connection
 *
 */
void
s_net_close(struct s_conn * conn);

#endif

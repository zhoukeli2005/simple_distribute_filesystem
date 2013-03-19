#include <s_net.h>
#include <s_packet.h>
#include <s_queue.h>

#include <s_ipc.h>

#include <s_mem.h>
#include <s_list.h>
#include <s_hash.h>
#include <s_common.h>

#include <sys/epoll.h>
#include <sys/types.h>
// for accept4
#define __USE_GNU
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define DEFAULT_PACKET_PENDING	32
struct packet_node {
	struct s_packet * pkt;
	char * p;
	int curr;
	int total;
};

struct irpc_param {
	unsigned int req_id;

	void * ud;
	S_NET_RPC_CALLBACK callback;
};

#define DEFAULT_READ_BUFFER_SZ	1024

#define DEFAULT_EPOLL_EVENTS_SZ	1024

struct s_conn {
	struct s_net * net;

	int fd;

	// information
	char ip[15];
	int port;

	// write queue
	struct s_queue * write_queue;

	// read buffer
	char * read_buf;
	int read_buf_p;
	int read_buf_sz;
	int read_buf_curr;

	// list for create/destroy
	struct s_list link;

	// udata
	void * udata;

	// rpc
	unsigned int req_id;
	struct s_hash * rpc_hash;
};

struct s_net {
	// epoll fd
	int efd;

	struct epoll_event * events;
	int events_sz;

	int nfd;

	// listen fd
	int lfd;
	int listen_port;

	// free conn
	struct s_list conn_list;
	int nconn;
	int nconn_free;

	// callback
	S_NET_CALLBACK callback;

	// udata
	void * udata;
}g_net;

/* -- add/rm fd to epoll -- */
static int iadd_fd(struct s_net * net, int fd, void * data);
static int irm_fd(struct s_net * net, int fd);

/* -- add/rm connection to epoll -- */
static int iadd_conn(struct s_net * net, struct s_conn * conn);
static int irm_conn(struct s_net * net, struct s_conn * conn);

/* -- get/put connection -- */
static struct s_conn * iget_conn(struct s_net * net);
static void iput_conn(struct s_net * net, struct s_conn * conn);

/* -- close a connection from other end -- */
#define iclose_conn_positive(net, conn)	\
	if(net->callback) {	\
		net->callback(conn, S_NET_CONN_CLOSED, net->udata);	\
	}	\
	irm_conn(net, conn);	\
	iput_conn(net, conn);

/* -- read/write connection -- */
static int iread_conn(struct s_net * net, struct s_conn * conn);
static int iwrite_conn(struct s_net * net, struct s_conn * conn);
static struct s_packet * inext_packet(struct s_net * net, struct s_conn * conn);

/* -- accept new connections -- */
static void ilisten_accept(struct s_net * net);

struct s_net * s_net_create(int listen_port, S_NET_CALLBACK callback, void * udata)
{
	struct s_net * net = &g_net;
	if((net->efd = epoll_create(1)) < 0) {
		s_log("net->efd create error!");
		perror("epoll_create");
		return NULL;
	}

	net->lfd = net->listen_port = -1;
	if(listen_port > 0) {
		net->listen_port = listen_port;
		
		net->lfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if(net->lfd < 0) {
			s_log("net->lfd create error!");
			perror("net->lfd = socket()");
			return NULL;
		}

		// set reusable
		int flag = 1;
		if(setsockopt(net->lfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
			s_log("net->lfd set reusable error!");
			perror("set reusable");
			return NULL;
		}

		struct sockaddr_in addr = {
			.sin_family = AF_INET,
			.sin_port = htons(listen_port),
			.sin_addr.s_addr = INADDR_ANY
		};

		if(bind(net->lfd, (struct sockaddr *)&addr, (socklen_t)(sizeof(struct sockaddr_in))) < 0) {
			s_log("net->lfd bind error!");
			perror("bind net->lfd");
			return NULL;
		}

		if(listen(net->lfd, 128) < 0) {
			s_log("net->lfd listen error!");
			perror("listen net->lfd");
			return NULL;
		}

		// epoll add
		iadd_fd(net, net->lfd, NULL);

		s_log("listen at:%d", net->listen_port);
	}

	s_list_init(&net->conn_list);
	net->nconn = 0;
	net->nconn_free = 0;

	net->nfd = 0;
	net->events = s_malloc(struct epoll_event, DEFAULT_EPOLL_EVENTS_SZ);
	net->events_sz = DEFAULT_EPOLL_EVENTS_SZ;

	net->callback = callback;
	net->udata = udata;

	return net;
}

int s_net_poll(struct s_net * net, int msec)
{
	if(net->nfd > net->events_sz) {
		struct epoll_event * old = net->events;
		net->events_sz = s_roundup(net->nfd + 1, 64);
		net->events = s_malloc(struct epoll_event, net->events_sz);
		s_free(old);
	}
	int n = epoll_wait(net->efd, net->events, net->events_sz, msec);
	if(n < 0) {
		switch(errno) {
			case EINTR:
				// interrupt by a signal, no problem
				return 0;
			default:
				return -1;
		}
		return -1;
	}
	int i;
	for(i = 0; i < n; ++i) {
		struct epoll_event * event = &net->events[i];
		if(event->data.ptr == NULL) {
			// listen event
			ilisten_accept(net);
			continue;
		}
		struct s_conn * conn = (struct s_conn *)event->data.ptr;
		if(event->events & EPOLLIN) {
			// new data comes...
			if(iread_conn(net, conn) < 0) {
				s_log("iread_conn(...) < 0. close conn!");
				iclose_conn_positive(net, conn);
				continue;
			}
			struct s_packet * pkt = inext_packet(net, conn);
			while(pkt) {
				unsigned int fun = s_packet_get_fun(pkt);
				// 1. it's a rpc back packet
				if(fun == S_PKT_TYPE_RPC_BACK_) {
					unsigned int req = s_packet_get_req(pkt);
					struct irpc_param * param = s_hash_get_num(conn->rpc_hash, req);
					if(param) {
						param->callback(conn, pkt, param->ud);
						s_hash_del_num(conn->rpc_hash, req);
					} else {
						s_log("[Warning] rpc request back, miss rpc_param(id:%u)", req);
					}
				} else {
				// 2. it's a normal packet
					if(net->callback) {
						net->callback(conn, pkt, net->udata);
					}
				}
				
				s_packet_drop(pkt);

				pkt = inext_packet(net, conn);
			}
			continue;
		}

		if(event->events & EPOLLOUT) {
			// can send ...
			if(iwrite_conn(net, conn) < 0) {
				s_log("iwrite_conn(...) < 0. close conn!");

				iclose_conn_positive(net, conn);
			}
			continue;
		}

		// error occured
		s_log("closed conn");

		iclose_conn_positive(net, conn);
	}

	return n;
}

void s_net_send(struct s_conn * conn, struct s_packet * pkt)
{
	struct packet_node * pn = s_queue_push(conn->write_queue);
	if(!pn) {
		s_log("no mem for send!");
		return;
	}

	s_packet_grab(pkt);
	pn->pkt = pkt;
	pn->p = s_packet_data_p(pkt);
	pn->total = s_packet_size(pkt);
	pn->curr = 0;

	if(iwrite_conn(conn->net, conn) < 0) {
		s_log("iwrite_conn(...) < 0. close conn!");
		iclose_conn_positive(conn->net, conn);
	}
}

void s_net_close(struct s_conn * conn)
{
	struct s_net * net = conn->net;
	if(net->callback) {
		net->callback(conn, S_NET_CONN_CLOSING, net->udata);
	}

	irm_conn(net, conn);
	iput_conn(net, conn);
}

struct s_conn * s_net_connect(struct s_net * net, const char * ip, int port)
{
	s_log("connecting to [%s : %d] ...", ip, port);

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = inet_addr(ip)
	};
	socklen_t len = sizeof(struct sockaddr_in);

	int fd = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(fd, (struct sockaddr *)&addr, len) < 0) {
		perror("connect");
		s_log("connect failed!");
		return NULL;
	}

	s_log("connect ok!");

	// set nonblock
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	struct s_conn * conn = iget_conn(net);
	if(!conn) {
		s_log("no mem for conn!");
		close(fd);
		return NULL;
	}

	conn->fd = fd;

	memcpy(conn->ip, ip, strlen(ip) + 1);
	conn->port = port;

	iadd_conn(net, conn);

	return conn;
}

const char * s_net_ip(struct s_conn * conn)
{
	return conn->ip;
}

int s_net_port(struct s_conn * conn)
{
	return conn->port;
}

void s_net_set_udata(struct s_conn * conn, void * d)
{
	conn->udata = d;
}

void * s_net_get_udata(struct s_conn * conn)
{
	return conn->udata;
}

static int iadd_fd(struct s_net * net, int fd, void * data)
{
	struct epoll_event event = {
		.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLET,
		.data.ptr = data
	};
	if(epoll_ctl(net->efd, EPOLL_CTL_ADD, fd, &event) < 0) {
		perror("epoll add");
		return -1;
	}
	net->nfd++;
	return 0;
}

static int irm_fd(struct s_net * net, int fd)
{
	if(epoll_ctl(net->efd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		perror("epoll rm");
		return -1;
	}
	close(fd);
	net->nfd--;
	return 0;
}

static int iadd_conn(struct s_net * net, struct s_conn * conn)
{
	return iadd_fd(net, conn->fd, conn);
}

static int irm_conn(struct s_net * net, struct s_conn * conn)
{
	irm_fd(net, conn->fd);

	s_list_del(&conn->link);
	s_list_insert(&net->conn_list, &conn->link);

	// remove packets
	while(1) {
		struct packet_node * pn = s_queue_peek(conn->write_queue);
		if(!pn) {
			break;
		}
		s_packet_drop(pn->pkt);
		s_queue_pop(conn->write_queue);
	}
	return 0;
}

static struct s_conn * iget_conn(struct s_net * net)
{
	struct s_conn * conn = NULL;
	if(!s_list_empty(&net->conn_list)) {
		struct s_list * l = s_list_first(&net->conn_list);
		s_list_del(l);
		conn = s_list_entry(l, struct s_conn, link);
		net->nconn_free--;
	} else {
		conn = s_malloc(struct s_conn, 1);
		if(!conn) {
			s_log("no mem!");
			return NULL;
		}
		net->nconn++;
	}

	conn->net = net;
	conn->fd = -1;
	conn->ip[0] = 0;
	conn->port = 0;
	conn->udata = NULL;

	conn->req_id = 1;
	conn->rpc_hash = s_hash_create(sizeof(struct irpc_param), 16);
	if(!conn->rpc_hash) {
		s_log("no mem for conn->rpc_hash");
		return NULL;
	}

	s_list_init(&conn->link);


	// reuse write_queue
	if(!conn->write_queue) {
		conn->write_queue = s_queue_create(sizeof(struct packet_node), DEFAULT_PACKET_PENDING);
		if(!conn->write_queue) {
			s_log("no mem for conn->write_queue!");
			return NULL;
		}
	}
	s_queue_clear(conn->write_queue);

	// reuse read_buf
	if(!conn->read_buf) {
		conn->read_buf = s_malloc(char, DEFAULT_READ_BUFFER_SZ);
		conn->read_buf_sz = DEFAULT_READ_BUFFER_SZ;
		if(!conn->read_buf) {
			s_log("no mem for conn->read_buf!");
			return NULL;
		}
	}
	conn->read_buf_p = 0;
	conn->read_buf_curr = 0;

	return conn;
}

static void iput_conn(struct s_net * net, struct s_conn * conn)
{
	if(conn->rpc_hash) {
		s_hash_destroy(conn->rpc_hash);
	}
	s_list_insert(&net->conn_list, &conn->link);
	net->nconn_free++;
}

static int iread_conn(struct s_net * net, struct s_conn * conn)
{
	int ndata;
	if(ioctl(conn->fd, FIONREAD, &ndata) < 0) {
		s_log("conn(%s:%d) ioctl error", conn->ip, conn->port);
		perror("ioctl");
		return -1;
	}
	if(ndata <= 0) {
		s_log("conn ioctl ndata(%d) <= 0", ndata);
		return -1;
	}
	if(conn->read_buf_sz - conn->read_buf_p < ndata) {
		char * old = conn->read_buf;
		conn->read_buf_sz = s_roundup(ndata * 2 + 1, 1024);
		conn->read_buf = s_malloc(char, conn->read_buf_sz);
		if(!conn->read_buf) {
			s_log("no mem!");
			return -1;
		}
		memcpy(conn->read_buf, old, conn->read_buf_p);
		s_free(old);
	}
	while(1) {
		int nread;
again:
		nread = read(conn->fd, &conn->read_buf[conn->read_buf_p], conn->read_buf_sz - conn->read_buf_p);
		if(nread < 0) {
			switch(errno) {
				case EINTR:
					goto again;
				case EAGAIN:
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
				case EWOULDBLOCK:
#endif
					goto out;
				default:
					return -1;
			}
			return -1;
		}
		conn->read_buf_p += nread;
		if(nread == 0) {
			break;
		}
	}
out:
	return 0;
}

static struct s_packet * inext_packet(struct s_net * net, struct s_conn * conn)
{
	struct s_packet * pkt = s_packet_create_from(&conn->read_buf[conn->read_buf_curr], conn->read_buf_p - conn->read_buf_curr);
	if(!pkt) {
		if(conn->read_buf_curr > 0) {
			int sz = conn->read_buf_p - conn->read_buf_curr;
			memmove(conn->read_buf, &conn->read_buf[conn->read_buf_curr], sz);
			conn->read_buf_p = sz;
			conn->read_buf_curr = 0;
		}
		return NULL;
	}
	conn->read_buf_curr += s_packet_size(pkt);
	return pkt;
}

static int iwrite_conn(struct s_net * net, struct s_conn * conn)
{
	while(1) {
		struct packet_node * pn;
again:
		pn = (struct packet_node *)s_queue_peek(conn->write_queue);
		if(!pn) {
			break;
		}
		int nsend = write(conn->fd, &pn->p[pn->curr], pn->total - pn->curr);
		if(nsend < 0) {
			switch(errno) {
				case EAGAIN:
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
				case EWOULDBLOCK:
#endif
					// ok
					return 0;

				case EINTR:
					// signal, again
					goto again;
				default:
					// error
					perror("write");
					break;
			}
			return -1;
		}
		if(!nsend) {
			// nothing write ? not EAGAIN ?
			break;
		}

		pn->curr += nsend;
		if(pn->curr >= pn->total) {
			// send a packet completely
			assert(pn->curr == pn->total);
			s_packet_drop(pn->pkt);
			s_queue_pop(conn->write_queue);
		}
	}
	return 0;
}

static void ilisten_accept(struct s_net * net)
{
	while(1) {
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(struct sockaddr_in);
		int fd;
again:
		fd = accept4(net->lfd, (struct sockaddr *)&addr, &addrlen, SOCK_NONBLOCK);
		if(fd < 0) {
			// accept4 failed
			switch(errno) {
				case EAGAIN:
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
				case EWOULDBLOCK:
#endif
					goto out;

				case EINTR:
					goto again;

				default:
					s_log("accept4 error!");
					perror("something error");
					goto out;
			}
			break;
		}
		struct s_conn * conn = iget_conn(net);
		if(!conn) {
			goto out;
		}
		conn->fd = fd;

		const char * ip = inet_ntoa(addr.sin_addr);
		memcpy(conn->ip, ip, s_min(strlen(ip) + 1, 15));
		conn->port = htons(addr.sin_port);
		s_log("accept new connection - %s:%d", conn->ip, conn->port);

		iadd_conn(net, conn);

		if(net->callback) {
			net->callback(conn, S_NET_CONN_ACCEPT, net->udata);
		}
	}
out:
	return;
}

/* -------------- RPC --------------- */
int s_net_rpc_call(struct s_conn * conn, struct s_packet * pkt, void * d, S_NET_RPC_CALLBACK callback)
{
	if(!callback) {
		s_log("[Warning] s_net_rpc_call : missing callback!");
		s_net_send(conn, pkt);
		return 0;
	}

	unsigned int req_id = conn->req_id++;

	struct irpc_param * param = s_hash_set_num(conn->rpc_hash, req_id);
	if(!param) {
		s_log("[Error] no mem!");
		return -1;
	}
	param->req_id = req_id++;
	if(s_packet_set_req(pkt, param->req_id) < 0) {
		s_log("[Error] s_packet_set_req failed:%u", param->req_id);
		return -1;
	}
	param->ud = d;
	param->callback = callback;

	s_net_send(conn, pkt);

	return 0;
}

void s_net_rpc_ret(struct s_conn * conn, unsigned int req_id, struct s_packet * pkt)
{
	s_packet_set_fun(pkt, S_PKT_TYPE_RPC_BACK_);
	s_packet_set_req(pkt, req_id);

	s_net_send(conn, pkt);
}


#ifndef s_ipc_h_
#define s_ipc_h_

#include <s_packet.h>

enum S_ENUM_PKT_TYPE {
	S_PKT_TYPE_RPC_BACK_ = 1,

	S_PKT_TYPE_IDENTIFY,
	S_PKT_TYPE_IDENTIFY_BACK,
	S_PKT_TYPE_PING,
	S_PKT_TYPE_PONG,

	S_PKT_TYPE_USER_DEF_BEGIN_,

	// begin user-define message

	// create file
		S_PKT_TYPE_CREATE,				// client --> mserv
		S_PKT_TYPE_CREATE_CHECK_AUTH,			// mserv --> other mserv
		S_PKT_TYPE_CREATE_CHECK_AUTH_BACK,		// other mserv --> mserv
		S_PKT_TYPE_CREATE_DECIDE,			// mserv --> other mserv
		S_PKT_TYPE_CREATE_CANCEL,			// mserv --> other mserv
		S_PKT_TYPE_CREATE_METADATA,			// main mserv --> data serv / mserv / slave mserv
		S_PKT_TYPE_CREATE_METADATA_ACCEPT,		// dserv --> main mserv
		S_PKT_TYPE_CREATE_BACK,				// mserv --> client

	// expand file size


	// end
	S_PKT_TYPE_MAX_
};

#define s_packet(req, fun, sz)	({	\
		struct s_packet * __pkt = s_packet_create(8 + sz);	\
		s_packet_write_uint(__pkt, req);	\
		s_packet_write_uint(__pkt, fun);	\
		__pkt; })

#define s_packet_read_fun(pkt, out) ({	\
	int __r = 1;	\
	__r = s_packet_seek(pkt, 4);	\
	__r = __r >= 0 && s_packet_read_uint(pkt, out) >= 0;	\
	__r; })

#define s_packet_write_fun(pkt, fun) ({	\
	int __r = 1;	\
	__r = s_packet_seek(pkt, 4);	\
	__r = __r >= 0 && s_packet_write_uint(pkt, fun) >= 0;	\
	__r; })

#define s_packet_read_req(pkt, out) ({	\
	int __r = 1;	\
	s_packet_seek(pkt, 0);	\
	__r = s_packet_read_uint(pkt, out);	\
	__r = __r >= 0 && s_packet_seek(pkt, 8) >= 0;	\
	__r; })

#define s_packet_write_req(pkt, req) ({	\
	int __r = 1;	\
	s_packet_seek(pkt, 0);	\
	__r = s_packet_write_uint(pkt, req);	\
	__r = __r >= 0 && s_packet_seek(pkt, 8) >= 0;	\
	__r; })

#endif


#ifndef s_ipc_h_
#define s_ipc_h_

#include <s_packet.h>

enum S_ENUM_PKT_TYPE {
	S_PKT_TYPE_IDENTIFY = 1,
	S_PKT_TYPE_IDENTIFY_BACK,
	S_PKT_TYPE_PING,
	S_PKT_TYPE_PONG,

	S_PKT_TYPE
};

#define s_ipc_pkt_identify(p, type, id, pwd)	\
	do {	\
		p = s_packet_create(16);	\
		s_packet_write_int(p, S_PKT_TYPE_IDENTIFY);	\
		s_packet_write_uint(p, pwd);	\
		s_packet_write_int(p, type);	\
		s_packet_write_int(p, id);	\
	} while(0)

#define s_ipc_pkt_identify_back(p, ret)	\
	do {	\
		p = s_packet_create(8);	\
		s_packet_write_int(p, S_PKT_TYPE_IDENTIFY_BACK);	\
		s_packet_write_int(p, ret);	\
	} while(0)


#define s_ipc_pkt_ping(p, sec, usec)	\
	do {	\
		p = s_packet_create(12);	\
		s_packet_write_int(p, S_PKT_TYPE_PING);	\
		s_packet_write_uint(p, sec);	\
		s_packet_write_uint(p, usec);	\
	} while(0)

#define s_ipc_pkt_pong(p)	\
	do {	\
		s_packet_seek(p, 0);	\
		s_packet_write_int(p, S_PKT_TYPE_PONG);	\
	} while(0)


#endif


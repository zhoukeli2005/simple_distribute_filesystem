#ifndef s_servg_pkt_h_
#define s_servg_pkt_h_

#include <s_packet.h>
#include <s_ipc.h>

#define s_servg_pkt_identify(type, id, pwd)	({	\
		struct s_packet * __p = s_packet(0, S_PKT_TYPE_IDENTIFY, 12);	\
		s_packet_write_uint(__p, pwd);	\
		s_packet_write_int(__p, type);	\
		s_packet_write_int(__p, id);	\
		__p; })

#define s_servg_pkt_identify_back(ret)	({	\
		struct s_packet * __p = s_packet(0, S_PKT_TYPE_IDENTIFY_BACK, 4);	\
		s_packet_write_int(__p, ret);	\
		__p; })


#define s_servg_pkt_ping(sec, usec)	({	\
		struct s_packet * __p = s_packet(0, S_PKT_TYPE_PING, 8);	\
		s_packet_write_uint(__p, sec);	\
		s_packet_write_uint(__p, usec);	\
		__p; })

#define s_servg_pkt_pong(p)	\
	do {	\
		s_packet_seek(p, 4);	\
		s_packet_write_int(p, S_PKT_TYPE_PONG);	\
	} while(0)


#endif


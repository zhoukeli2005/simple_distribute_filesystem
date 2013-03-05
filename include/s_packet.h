#ifndef s_packet_h_
#define s_packet_h_

#include <s_string.h>

struct s_packet;

struct s_packet *
s_packet_create(int size);

struct s_packet *
s_packet_create_from(char * p, int len);


int
s_packet_size(struct s_packet * pkt);

char *
s_packet_data_p(struct s_packet * pkt);

int
s_packet_seek(struct s_packet * pkt, int offset);

int
s_packet_read_char(struct s_packet * pkt, char * c);

int
s_packet_read_short(struct s_packet * pkt, short * s);

int
s_packet_read_ushort(struct s_packet * pkt, unsigned short * us);

int
s_packet_read_int(struct s_packet * pkt, int * i);

int
s_packet_read_uint(struct s_packet * pkt, unsigned int * u);

struct s_string *
s_packet_read_string(struct s_packet * pkt);

int
s_packet_eof(struct s_packet * pkt);

int
s_packet_write_char(struct s_packet * pkt, char c);

int
s_packet_write_short(struct s_packet * pkt, short s);

int
s_packet_write_ushort(struct s_packet * pkt, unsigned short s);

int
s_packet_write_int(struct s_packet * pkt, int i);

int
s_packet_write_uint(struct s_packet * pkt, unsigned int u);

int
s_packet_write_s(struct s_packet * pkt, const char * p, int count);

int
s_packet_write_string(struct s_packet * pkt, struct s_string * s);

#define s_packet_read(pkt, out, type, error)	\
	if(s_packet_read_##type(pkt, out) < 0) {	\
		s_log("s_packet_read_" #type "error!");	\
		goto error;\
	}

/*
 *	Increase Reference Count
 *
 */
void
s_packet_grab(struct s_packet * pkt);


/*
 *	Decrease Reference Count
 *
 */
void
s_packet_drop(struct s_packet * pkt);

#endif

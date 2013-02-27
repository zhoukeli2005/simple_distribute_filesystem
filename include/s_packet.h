#ifndef s_packet_h_
#define s_packet_h_

struct s_packet;

struct s_packet *
s_packet_create(int size);

int
s_packet_read_char(struct s_packet * pkt, char * c);

int
s_packet_read_short(struct s_packet * pkt, short * s);

int
s_packet_read_int(struct s_packet * pkt, int * i);

int
s_packet_read_string(struct s_packet * pkt, char * buf, int count);

int
s_packet_eof(struct s_packet * pkt);

int
s_packet_write_char(struct s_packet * pkt, char c);

int
s_packet_write_short(struct s_packet * pkt, short s);

int
s_packet_write_int(struct s_packet * pkt, int i);

int
s_packet_write_string(struct s_packet * pkt, char * buf, int count);

#endif

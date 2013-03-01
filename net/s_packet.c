#include <s_packet.h>
#include <s_mem.h>
#include <s_common.h>

/*
 *	Packet - | size (4 bytes) | ... data ( var size ) ... |
 *
 */

struct s_packet {
	int size;
	int pos;
	int ref_count;
};

struct s_packet * s_packet_create(int size)
{
	struct s_packet * pkt = (struct s_packet *)s_malloc(char, sizeof(struct s_packet) + size);
	pkt->size = size;
	pkt->pos = 0;
	pkt->ref_count = 1;

	return pkt;
}

struct s_packet * s_packet_create_from(char * p, int len)
{
	if(len < 4) {
		// too small
		return NULL;
	}
	int sz = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
	if(len < sz) {
		// not complete
		return NULL;
	}

	struct s_packet * pkt = s_packet_create(sz);
	char * data = (char *)(pkt + 1);
	memcpy(data, p, sz);

	pkt->size = sz;
	return pkt;
}

void s_packet_get(struct s_packet * pkt)
{
	pkt->ref_count++;
}

void s_packet_put(struct s_packet * pkt)
{
	pkt->ref_count--;
	if(pkt->ref_count == 0) {
		s_free(pkt);
	}

	if(pkt->ref_count < 0) {
		s_log("double-free packet!");
	}
}

int s_packet_size(struct s_packet * pkt)
{
	return pkt->size;
}

char * s_packet_data_p(struct s_packet * pkt)
{
	return (char*)(pkt + 1);
}


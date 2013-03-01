#include <s_packet.h>
#include <s_mem.h>
#include <s_common.h>

/*
 *	Packet - | size (4 bytes) | ... data ( var size ) ... |
 *
 */

struct s_packet {
	int size;
	int pos;	// for read/write
	int ref_count;
};

struct s_packet * s_packet_create(int size)
{
	size += 4;

	struct s_packet * pkt = (struct s_packet *)s_malloc(char, sizeof(struct s_packet) + size);
	pkt->size = size;
	pkt->pos = 4;
	pkt->ref_count = 1;

	char * p = (char *)(pkt + 1);
	p[0] = size & 0xFF;
	p[1] = (size >> 8) & 0xFF;
	p[2] = (size >> 16) & 0xFF;
	p[3] = (size >> 24) & 0xFF;

	return pkt;
}

struct s_packet * s_packet_create_from(char * p, int len)
{
	if(len < 4) {
		// too small
		return NULL;
	}
	int sz = p[0] | ((p[1] << 8) & 0xFF00) | ((p[2] << 16) & 0xFF0000) | ((p[3] << 24) & 0xFF000000);
	assert(sz > 0);
	if(len < sz) {
		// not complete
		return NULL;
	}

	struct s_packet * pkt = s_packet_create(sz - 4);
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


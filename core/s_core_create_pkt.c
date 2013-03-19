#include "s_core_create_pkt.h"

struct s_packet * s_core_create_pkt_create(struct s_core * core, struct s_core_metadata_param * param)
{
	int sz = s_packet_size_for_string(param->fname) +
		s_packet_size_for_uint(param->size.low) +
		s_packet_size_for_uint(param->size.high);

	struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_CREATE, sz);
	if(!pkt) {
		return NULL;
	}
	s_packet_write(pkt, param->fname, string, label_error);
	s_packet_write(pkt, param->size.low, uint, label_error);
	s_packet_write(pkt, param->size.high, uint, label_error);

	return pkt;

label_error:

	s_packet_drop(pkt);

	return NULL;
}


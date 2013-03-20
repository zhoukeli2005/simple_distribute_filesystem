#include "s_core.h"
#include "s_core_param.h"
#include "s_core_create.h"
#include "s_core_pkt.h"

void s_core_dserv_create_metadata(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	s_log("[LOG] create file here!");
	struct s_core * core = (struct s_core *)ud;
	struct s_file_meta_data * fmd = s_core_pkt_to_fmd(core, pkt);
	int ret = 1;
	if(!fmd) {
		s_log("[LOG] create metadata : read from packet failed!");
		ret = 0;
		goto label_rpc_ret;
	}

	s_log("fname:%s, desc:%u-%u, size:%u-%u", s_string_data_p(fmd->fname),
			fmd->fdesc.file_id, fmd->fdesc.file_version,
			fmd->fsize.high, fmd->fsize.low);

	s_log("nblocks:%d", fmd->nblocks);
	int i;
	for(i = 0; i < fmd->nblocks; ++i) {
		s_log("%d-%d", fmd->blocks[i].locate[0], fmd->blocks[i].locate[1]);
	}

label_rpc_ret:
	{
	// rpc ret 
		struct s_packet * pkt_ret = s_packet_create(4);
		s_packet_write_int(pkt_ret, ret);
		s_servg_rpc_ret(serv, s_packet_get_req(pkt), pkt_ret);
		s_packet_drop(pkt_ret);
	}
}


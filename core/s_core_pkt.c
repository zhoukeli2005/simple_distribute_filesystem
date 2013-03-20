#include "s_core_pkt.h"
#include "s_core_param.h"

struct s_packet * s_core_pkt_from_mdp(struct s_core * core, struct s_core_metadata_param * param)
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

struct s_packet * s_core_pkt_from_fmmd( struct s_core * core, struct s_file_meta_meta_data * mmd )
{
	int sz = s_packet_size_for_string(mmd->fname) +
		s_packet_size_for_uint(mmd->fdesc.file_id) +
		s_packet_size_for_uint(mmd->fdesc.file_version) +
		s_packet_size_for_uint(mmd->nmserv) +
		s_packet_size_for_uint() * mmd->nmserv;

	struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_CREATE_META_META_DATA, sz);
	s_packet_write_string(pkt, mmd->fname);
	s_packet_write_uint(pkt, mmd->fdesc.file_id);
	s_packet_write_uint(pkt, mmd->fdesc.file_version);
	s_packet_write_uint(pkt, mmd->nmserv);
	int i;
	for(i = 0; i < mmd->nmserv; ++i) {
		s_packet_write_uint(pkt, mmd->mservs[i]);
	}

	return pkt;
}

struct s_file_meta_meta_data * s_core_pkt_to_fmmd(struct s_core * core, struct s_packet * pkt)
{
	struct s_string * fname = NULL;
	struct s_file_desc fdesc;
	unsigned int nmserv;

	struct s_file_meta_meta_data * fmmd = NULL;

	s_packet_read(pkt, &fname, string, label_error);
	s_packet_read(pkt, &fdesc.file_id, uint, label_error);
	s_packet_read(pkt, &fdesc.file_version, uint, label_error);
	s_packet_read(pkt, &nmserv, uint, label_error);

	fmmd = s_file_meta_meta_data_create( core, nmserv );
	if(!fmmd) {
		s_log("[Error] no mem for file_meta-meta-data!");
		goto label_error;
	}
	fmmd->fname = fname;
	fmmd->fdesc = fdesc;
	fmmd->nmserv = nmserv;
	int i;
	for(i = 0; i < nmserv; ++i) {
		s_packet_read(pkt, &fmmd->mservs[i], uint, label_error);
	}

	return fmmd;

label_error:
	if(fname) {
		s_string_drop(fname);
	}

	if(fmmd) {
		s_file_meta_meta_data_destroy(fmmd);
		return NULL;
	}
}

struct s_packet * s_core_pkt_from_fmd(struct s_core * core, struct s_file_meta_data * md)
{
	int sz = s_packet_size_for_string(md->fname) +
		s_packet_size_for_uint(md->fdesc.file_id) +
		s_packet_size_for_uint(md->fdesc.file_version) +
		s_packet_size_for_uint(md->fsize.high) +
		s_packet_size_for_uint(md->fsize.low) +
		s_packet_size_for_uint(md->nblocks) +
		(s_packet_size_for_ushort() * 2) * md->nblocks;

	struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_CREATE_METADATA, sz);
	s_packet_write_string(pkt, md->fname);
	s_packet_write_uint(pkt, md->fdesc.file_id);
	s_packet_write_uint(pkt, md->fdesc.file_version);
	s_packet_write_uint(pkt, md->fsize.high);
	s_packet_write_uint(pkt, md->fsize.low);
	s_packet_write_uint(pkt, md->nblocks);
	int i;
	for(i = 0; i < md->nblocks; ++i) {
		s_packet_write_ushort(pkt, md->blocks[i].locate[0]);
		s_packet_write_ushort(pkt, md->blocks[i].locate[1]);
	}

	return pkt;
}

struct s_file_meta_data * s_core_pkt_to_fmd(struct s_core * core, struct s_packet * pkt)
{
	struct s_string * fname = NULL;
	struct s_file_desc fdesc;
	struct s_file_size fsize;
	unsigned int nblocks;

	struct s_file_meta_data * md = NULL;

	s_packet_seek(pkt, 0);
	s_packet_read(pkt, &fname, string, label_error);
	s_packet_read(pkt, &fdesc.file_id, uint, label_error);
	s_packet_read(pkt, &fdesc.file_version, uint, label_error);
	s_packet_read(pkt, &fsize.high, uint, label_error);
	s_packet_read(pkt, &fsize.low, uint, label_error);
	s_packet_read(pkt, &nblocks, uint, label_error);

	md = s_file_meta_data_create(core, nblocks);
	if(!md) {
		s_string_drop(fname);
		return NULL;
	}
	md->fname = fname;
	md->fdesc = fdesc;
	md->fsize = fsize;
	md->nblocks = nblocks;
	int i;
	for(i = 0; i < nblocks; ++i) {
		unsigned short int tmp;
		s_packet_read(pkt, &tmp, ushort, label_error);
		md->blocks[i].locate[0] = tmp;

		s_packet_read(pkt, &tmp, ushort, label_error);
		md->blocks[i].locate[1] = tmp;
	}

	return md;

label_error:
	if(fname) {
		s_string_drop(fname);
	}
	if(md) {
		s_file_meta_data_destroy(md);
	}
	return NULL;
}


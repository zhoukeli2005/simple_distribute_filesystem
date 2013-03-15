#include "s_core.h"
#include "s_core_param.h"
#include "s_core_create.h"

void s_core_mserv_create(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	unsigned int req_id;
	struct s_string * fname;
	struct s_file_size size;

	s_packet_read(pkt, &req_id, req, label_error);
	s_packet_read(pkt, &fname, string, label_error);
	s_packet_read(pkt, &size.low, uint, label_error);
	s_packet_read(pkt, &size.high, uint, label_error);

	s_log("create file : %p, size:%u-%u", s_string_data_p(fname), size.high, size.low);

label_error:
	return;
}

void s_core_mserv_create_check_auth(struct s_server * serv, struct s_packet * pkt, void * ud)
{
}

void s_core_mserv_create_check_auth_back(struct s_server * serv, struct s_packet * pkt, void * ud)
{
}

void s_core_mserv_create_cancel(struct s_server * serv, struct s_packet * pkt, void * ud)
{
}

void s_core_mserv_create_decide(struct s_server * serv, struct s_packet * pkt, void * ud)
{
}

void s_core_mserv_create_metadata(struct s_server * serv, struct s_packet * pkt, void * ud)
{
}

void s_core_mserv_create_md_accept(struct s_server * serv, struct s_packet * pkt, void * ud)
{
}


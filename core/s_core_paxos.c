#include <s_mem.h>
#include <s_array.h>
#include <s_hash.h>
#include <s_packet.h>
#include <s_server_group.h>

#include "s_core.h"
#include "s_core_paxos.h"

static struct s_paxos_proposal * s_paxos_proposal_create(struct s_core * core, struct s_string * topic, int value);
static struct s_paxos_proposal * s_paxos_proposal_grab( struct s_paxos_proposal * p );
static void s_paxos_proposal_drop(struct s_paxos_proposal * p);

static void ireceive_request(struct s_server * serv, struct s_packet * pkt, void * ud);
static void ireceive_response(struct s_server * serv, struct s_packet * pkt, void * ud);
static void ireceive_accept(struct s_server * serv, struct s_packet * pkt, void * ud);
static void ireceive_learn(struct s_server * serv, struct s_packet * pkt, void * ud);

static void ilearn(struct s_core * core, struct s_string * topic, int value, int serv_id);

struct s_paxos * s_paxos_create(struct s_core * core)
{
	struct s_paxos * paxos = s_malloc(struct s_paxos, 1);

	paxos->core = core;

	paxos->version.version = 0;
	paxos->version.nserv = 0;
	if(core->id == 1) {
		s_log("[LOG] init mserv 1");
		paxos->version.version = 1;
		paxos->version.nserv = 1;
		paxos->version.servs[0] = 1;
	}

	paxos->my_proposal = s_hash_create(sizeof(struct s_paxos_proposal *), 16);
	paxos->response = s_hash_create(sizeof(struct s_id), 16);
	paxos->accept = s_hash_create(sizeof(struct s_array *), 16);
	paxos->learn = s_hash_create(sizeof(struct s_array *), 16);

	paxos->id = 1;

	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_PAXOS_REQUEST, &ireceive_request);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_PAXOS_ACCEPT, &ireceive_accept);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_PAXOS_LEARN, &ireceive_learn);

	return paxos;
}

int s_paxos_is_in(struct s_paxos * paxos, int serv)
{
	int i;
	for(i = 0; i < paxos->version.nserv; ++i) {
		if(paxos->version.servs[i] == serv) {
			return 1;
		}
	}
	return 0;
}

static struct s_paxos_proposal * s_paxos_proposal_create(struct s_core * core, struct s_string * topic, int value)
{
	struct s_paxos * paxos = core->paxos;
	struct s_paxos_proposal * p = s_malloc(struct s_paxos_proposal, 1);
	memcpy(&p->version, &paxos->version, sizeof(struct s_paxos_version));

	p->core = core;
	p->ref_count = 1;

	p->topic = NULL;
	if(topic) {
		p->topic = s_string_grab(topic);
	}
	p->value = value;
	p->id.x = core->id;
	p->id.y = paxos->id++;

	p->response.count = 0;
	p->response.value = -1;
	p->response.max_id.x = 0;
	p->response.max_id.y = 0;

	return p;
}

static struct s_paxos_proposal * s_paxos_proposal_grab( struct s_paxos_proposal * p )
{
	p->ref_count++;
	return p;
}

static void s_paxos_proposal_drop(struct s_paxos_proposal * p)
{
	assert(p->ref_count > 0);
	p->ref_count--;
	if(p->ref_count == 0) {
		s_string_drop(p->topic);
		s_free(p);
	}
}

/* ---------- Start A Proposal ------------ */
void s_paxos_start(struct s_core * core, struct s_string * topic, int value)
{
	struct s_paxos * paxos = core->paxos;
	struct s_paxos_proposal * p = s_paxos_proposal_create(core, topic, value);

	// broadcast to all servs;
	struct s_packet * pkt;
	int i;
	s_packet_start_write(pkt, S_PKT_TYPE_PAXOS_REQUEST)
	{
		s_packet_wstring(p->topic);
		s_packet_wint(p->id.x);
		s_packet_wint(p->id.y);
		s_packet_wint(p->value);

		s_packet_wint(p->version.version);
		s_packet_wint(p->version.nserv);
		for(i = 0; i < paxos->version.nserv; ++i) {
			s_packet_wint(paxos->version.servs[i]);
		}
	}
	s_packet_end_write();

	for(i = 0; i < paxos->version.nserv; ++i) {
		int serv_id = paxos->version.servs[i];
		if(serv_id != core->id) {
			// not myself
			struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, serv_id);
			if(!serv) {
				s_log("[Warning] paxos_start, send(topic:%s) to serv:%d, missing!",
						s_string_data_p(topic), serv_id);
				continue;
			}
			
			s_servg_rpc_call(serv, pkt, s_paxos_proposal_grab(p), &ireceive_response, 1000);
		}
	}

	s_packet_drop(pkt);

	// response from myself
	struct s_array ** ppary = s_hash_get_str(paxos->accept, p->topic);
	struct s_id max_id = {0, 0};
	value = -1;
	for(i = 0; ppary && i < s_array_len(*ppary); ++i) {
		struct s_paxos_accept * tmp = s_array_at(*ppary, i);
		if(s_id_gt(&p->id, &tmp->id)) {
			if(s_id_gt(&tmp->id, &max_id)) {
				max_id = tmp->id;
				value = tmp->value;
			}
		}
	}
	if(value < 0) {
		s_paxos_response(core, p, NULL, -1);
	} else {
		s_paxos_response(core, p, &max_id, value);
	}

	// drop proposal
	s_paxos_proposal_drop(p);
}

static void ireceive_request(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = ud;
	struct s_paxos * paxos = core->paxos;
	struct s_paxos_proposal * p = s_paxos_proposal_create(core, NULL, -1);

	unsigned int req_id = s_packet_get_req(pkt);

	s_packet_read(pkt, &p->topic, string, label_error);
	s_packet_read(pkt, &p->id.x, int, label_error);
	s_packet_read(pkt, &p->id.y, int, label_error);
	s_packet_read(pkt, &p->value, int, label_error);
	s_packet_read(pkt, &p->version.version, int, label_error);
	s_packet_read(pkt, &p->version.nserv, int, label_error);
	int i;
	for(i = 0; i < p->version.nserv; ++i) {
		s_packet_read(pkt, &p->version.servs[i], int, label_error);
	}

	s_log("[LOG] receive request, topic:%s, value:%d, id(%d.%d), version(%d, %d), my_verison(%d, %d)",
			s_string_data_p(p->topic), p->value, p->id.x, p->id.y, p->version.version, p->version.nserv,
			paxos->version.version, paxos->version.nserv
	);

	if(paxos->version.version != p->version.version) {
		s_packet_start_write(pkt, 0)
		{
			s_packet_wint(S_PAXOS_RESPONSE_BAD_VERSION);
		}
		s_packet_end_write();
		
		s_servg_rpc_ret(serv, req_id, pkt);

		s_packet_drop(pkt);

		return;
	}

	struct s_array ** pp = s_hash_get_str(paxos->accept, p->topic);
	struct s_id max_id = {0, 0};
	int value = -1;
	for(i = 0; pp && i < s_array_len(*pp); ++i) {
		struct s_paxos_accept * tmp = s_array_at(*pp, i);
		if(s_id_gt(&p->id, &tmp->id)) {
			if(s_id_gt(&tmp->id, &max_id)) {
				max_id = tmp->id;
				value = tmp->value;
			}
		}
	}

	s_packet_start_write(pkt, 0)
	{
		if(value < 0) {
			s_packet_wint(S_PAXOS_RESPONSE_NO_VALUE);
		} else {
			s_packet_wint(S_PAXOS_RESPONSE_VALUE);
			s_packet_wint(max_id.x);
			s_packet_wint(max_id.y);
			s_packet_wint(value);
		}
	}
	s_packet_end_write();

	s_servg_rpc_ret(serv, req_id, pkt);

	s_packet_drop(pkt);

	// save response
	struct s_id * pid = s_hash_get_str(paxos->response, p->topic);
	if(pid) {
		if(s_id_gt(&p->id, pid)) {
			*pid = p->id;
		}
	} else {
		pid = s_hash_set_str(paxos->response, p->topic);
		*pid = p->id;
	}

label_error:
	s_paxos_proposal_drop(p);
}

static void ireceive_response(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_paxos_proposal * p = ud;
	struct s_core * core = p->core;
	struct s_paxos * paxos = core->paxos;
	s_used(paxos);

	if(!pkt) {
		s_log("[LOG] wait for response time out!");
		s_paxos_proposal_drop(p);
		return;
	}

	int suc;
	struct s_id max_id;
	int value;

	s_packet_read(pkt, &suc, int, label_error);
	if(suc == S_PAXOS_RESPONSE_BAD_VERSION) {
		s_log("[LOG]iresponse bad version(id(%d.%d), topic:%s)!", p->id.x, p->id.y, s_string_data_p(p->topic));
		s_paxos_proposal_drop(p);
		return;
	}

	if(suc == S_PAXOS_RESPONSE_VALUE) {
		s_packet_read(pkt, &max_id.x, int, label_error);
		s_packet_read(pkt, &max_id.y, int, label_error);
		s_packet_read(pkt, &value, int, label_error);

		if(s_id_gt(&max_id, &p->response.max_id)) {
			p->response.max_id = max_id;
			p->response.value = value;
		}

		s_paxos_response(core, p, &max_id, value);
	} else {
		s_paxos_response(core, p, NULL, -1);
	}

label_error:

	s_paxos_proposal_drop(p);
	return;
}

void s_paxos_response(struct s_core * core, struct s_paxos_proposal * p, struct s_id * id, int value)
{
	struct s_paxos * paxos = core->paxos;

	p->response.count++;

	if(id) {
		if(s_id_gt(id, &p->response.max_id)) {
			p->response.max_id = *id;
			p->response.value = value;
		}
	}

	if(p->response.count > p->version.nserv / 2) {
		s_log("[LOG] iresponse count:%d gt %d/2, max_id(%d.%d), value:%d,send accept ...",
				p->response.count, p->version.nserv, p->response.max_id.x, p->response.max_id.y,
				p->response.value);

		// send accept ...
		struct s_packet * pkt;
		s_packet_start_write(pkt, S_PKT_TYPE_PAXOS_ACCEPT)
		{
			s_packet_wstring(p->topic);
			s_packet_wint(p->id.x);
			s_packet_wint(p->id.y);
			s_packet_wint(p->value);
			s_packet_wint(p->version.version);
		}
		s_packet_end_write();

		int i;
		for(i = 0; i < paxos->version.nserv; ++i) {
			int serv_id = paxos->version.servs[i];
			struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, serv_id);
			if(!serv) {
				s_log("[LOG] s_paxos_response:send accept to serv:%d, missing!", serv_id);
				continue;
			}
			s_servg_send(serv, pkt);
		}

		s_packet_drop(pkt);
	}
}

static void ireceive_accept(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = ud;
	struct s_paxos * paxos = core->paxos;

	struct s_string * topic = NULL;
	struct s_id id;
	int value;
	int version;

	s_packet_read(pkt, &topic, string, label_error);
	s_packet_read(pkt, &id.x, int, label_error);
	s_packet_read(pkt, &id.y, int, label_error);
	s_packet_read(pkt, &value, int, label_error);
	s_packet_read(pkt, &version, int, label_error);

	if(version != paxos->version.version) {
		s_log("[LOG] accept : invalid version(%d != %d)", version, paxos->version.version);
		goto label_error;
	}

	struct s_id * pid = s_hash_get_str(paxos->response, topic);
	if(pid) {
		if(s_id_gt(pid, &id)) {
			s_log("[LOG] accept : smaller id(%d.%d) < (%d.%d)", id.x, id.y, pid->x, pid->y);
			goto label_error;
		}
	}

	struct s_array ** pp = s_hash_get_str(paxos->response, topic);
	if(!pp) {
		pp = s_hash_set_str(paxos->response, topic);
		*pp = s_array_create(sizeof(struct s_paxos_accept), 16);
	}

	struct s_array * tmp = *pp;
	struct s_paxos_accept * accept = s_array_push(tmp);

	accept->topic = topic;
	accept->id = id;
	accept->value = value;

	// save to myself -- learner
	ilearn(core, topic, value, core->id);

	// broadcast to every one
	s_packet_start_write(pkt, S_PKT_TYPE_PAXOS_LEARN)
	{
		s_packet_wstring(topic);
		s_packet_wint(value);
	}
	s_packet_end_write();
	int i;
	for(i = 0; i < paxos->version.nserv; ++i) {
		int serv_id = paxos->version.servs[i];
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, serv_id);
		if(!serv) {
			s_log("[LOG] broadcast to leaner(%d) missing", serv_id);
			continue;
		}

		s_servg_send(serv, pkt);
	}

	s_packet_drop(pkt);

	return;

label_error:
	if(topic) {
		s_string_drop(topic);
	}
	return;
}

static void ireceive_learn(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = ud;

	struct s_string * topic = NULL;
	int value;

	s_packet_read(pkt, &topic, string, label_error);
	s_packet_read(pkt, &value, int, label_error);

	s_log("[LOG] learn : topic(%s) = value(%d)", s_string_data_p(topic), value);

	ilearn(core, topic, value, s_servg_get_id(serv));

label_error:
	if(topic) {
		s_string_drop(topic);
	}
	return;
}

static void ilearn(struct s_core * core, struct s_string * topic, int value, int serv_id)
{
	struct s_paxos * paxos = core->paxos;
	struct s_hash ** pp = s_hash_get_str(paxos->learn, topic);
	if(!pp) {
		pp = s_hash_get_str(paxos->learn, topic);
		*pp = s_hash_create(sizeof(int), 16);
	}

	struct s_hash * p = *pp;
	int * pcount = s_hash_get_num(p, value);
	if(!pcount) {
		pcount = s_hash_set_num(p, value);
		*pcount = 0;
	}
	(*pcount)++;

	return;
}

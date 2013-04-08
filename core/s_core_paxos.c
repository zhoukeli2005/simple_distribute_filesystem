#include <s_mem.h>
#include <s_array.h>
#include <s_hash.h>
#include <s_packet.h>
#include <s_server_group.h>

#include "s_core.h"
#include "s_core_paxos.h"

/* ---  create/grab/drop Proposal --- */
static struct s_paxos_proposal * s_paxos_proposal_create(struct s_core * core, struct s_string * topic, int value);
static struct s_paxos_proposal * s_paxos_proposal_grab( struct s_paxos_proposal * p );
static void s_paxos_proposal_drop(struct s_paxos_proposal * p);

/* --- receive packet from others --- */
static void ireceive_request(struct s_server * serv, struct s_packet * pkt, void * ud);
static void ireceive_response(struct s_server * serv, struct s_packet * pkt, void * ud);
static void ireceive_accept(struct s_server * serv, struct s_packet * pkt, void * ud);
static void ireceive_learn(struct s_server * serv, struct s_packet * pkt, void * ud);
static void ireceive_learn_request(struct s_server * serv, struct s_packet * pkt, void * ud);
static void ireceive_add(struct s_server * serv, struct s_packet * pkt, void * ud);
static void ireceive_add_accept(struct s_server * serv, struct s_packet * pkt, void * ud);

/* --- do the job --- */
static void iresponse(struct s_core * core, struct s_paxos_proposal * p, struct s_id * id, int value);
static void iaccept(struct s_core * core, struct s_string * topic, struct s_id * id, int value, int version);
static void ilearn(struct s_core * core, struct s_string * topic, int value, struct s_id * id, int serv_id);

/* --- util ---- */
static int iget_new_version(struct s_string * topic)
{
	if(strncmp(s_string_data_p(topic), "version:", 8) == 0) {
		return atoi(s_string_data_p(topic) + 8);
	}
	return -1;
}

static void icheck_new_version(struct s_core * core);
static struct s_packet * idump_learn(struct s_core * core, int gen_pkt);

/* --- Main --- */
struct s_paxos * s_paxos_create(struct s_core * core)
{
	struct s_paxos * paxos = s_malloc(struct s_paxos, 1);

	paxos->core = core;

	paxos->version.version = 0;
	paxos->version.nserv = 0;
	paxos->response_version = 0;

	if(core->id == 1) {
		s_log("[LOG] init mserv 1");
		paxos->version.version = 1;
		paxos->version.nserv = 1;
		paxos->version.servs[0] = 1;
	}

	paxos->my_proposal = s_hash_create(sizeof(struct s_paxos_proposal *), 16);
	paxos->response = s_hash_create(sizeof(struct s_id), 16);
	paxos->accept = s_hash_create(sizeof(struct s_array *), 16);
	paxos->learn = s_hash_create(sizeof(struct s_hash *), 16);

	paxos->id = 1;

	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_PAXOS_REQUEST, &ireceive_request);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_PAXOS_ACCEPT, &ireceive_accept);
//	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_PAXOS_LEARN, &ireceive_learn);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_PAXOS_LEARN_REQUEST, &ireceive_learn_request);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_PAXOS_ADD_ME, &ireceive_add);
	s_servg_register(core->servg, S_SERV_TYPE_M, S_PKT_TYPE_PAXOS_ADD_ACCEPT, &ireceive_add_accept);

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
		if(p->topic) {
			s_string_drop(p->topic);
		}
		s_free(p);
	}
}

/* ---------- Start A Proposal ------------ */
void s_paxos_start(struct s_core * core, struct s_string * topic, int value, int type, int the_serv)
{
	struct s_paxos * paxos = core->paxos;
	if(!s_paxos_is_in(paxos, core->id)) {
		s_log("[Warning] s_paxos_start, I am not in the serv!");
		return;
	}
	struct s_paxos_proposal * p = s_paxos_proposal_create(core, topic, value);
	p->type = type;
	p->the_serv = the_serv;
	if(type == S_PAXOS_PROPOSAL_ADD) {
		int i;
		value = 0;
		for(i = 0; i < paxos->version.nserv; ++i) {
			if(the_serv == paxos->version.servs[i]) {
				s_log("[Warning] s_paxos_start, serv(%d) already in version!", the_serv);
				return;
			}
			value |= 1 << paxos->version.servs[i];
		}
		value |= 1 << the_serv;
	}
	if(type == S_PAXOS_PROPOSAL_RM) {
		int i;
		value = 0;
		for(i = 0; i < paxos->version.nserv; ++i) {
			if(the_serv != paxos->version.servs[i]) {
				value |= 1 << paxos->version.servs[i];
			}
		}
	}
	p->value = value;

	s_log("[Proposal] Phase 1 <Prepare Request>. Topic:(%s), ID(%d.%d), Value(%d), Version(%d)",
			s_string_data_p(topic), p->id.x, p->id.y, p->value, p->version.version);

	// broadcast to all servs;
	struct s_packet * pkt;
	int i;
	s_packet_start_write(pkt, S_PKT_TYPE_PAXOS_REQUEST)
	{
		s_packet_wstring(p->topic);
		s_packet_wint(p->id.x);
		s_packet_wint(p->id.y);
		s_packet_wint(p->value);

		s_packet_wint(p->type);
		s_packet_wint(p->the_serv);

		s_packet_wint(p->version.version);
		s_packet_wint(p->version.nserv);
		for(i = 0; i < paxos->version.nserv; ++i) {
			s_packet_wint(paxos->version.servs[i]);
		}
	}
	s_packet_end_write();

	for(i = 0; i < paxos->version.nserv; ++i) {
		int serv_id = paxos->version.servs[i];
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, serv_id);
		if(!serv) {
			s_log("[Warning] paxos_start, send(topic:%s) to serv:%d, missing!",
					s_string_data_p(topic), serv_id);
			continue;
		}
		
		s_servg_rpc_call(serv, pkt, s_paxos_proposal_grab(p), &ireceive_response, 1000);
	}

	s_packet_drop(pkt);

	// drop proposal
	s_paxos_proposal_drop(p);
}

void s_paxos_learn_all(struct s_core * core)
{
	struct s_paxos * paxos = core->paxos;

	if(paxos->learn) {
		s_hash_destroy(paxos->learn);
	}

	paxos->learn = s_hash_create(sizeof(struct s_array *), 16);

	struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_PAXOS_LEARN_REQUEST, 0);

	int c = 0;
	int i;
	for(i = 0; i < paxos->version.nserv; ++i) {
		int serv_id = paxos->version.servs[i];
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, serv_id);
		if(!serv) {
			s_log("[Warning] learn to serv:%d, missing!", serv_id);
			continue;
		}
		c++;
		s_servg_rpc_call(serv, pkt, core, ireceive_learn, 1000);
	}

	paxos->learn_req.count = c;

	s_packet_drop(pkt);
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

	s_packet_read(pkt, &p->type, int, label_error);
	s_packet_read(pkt, &p->the_serv, int, label_error);

	s_packet_read(pkt, &p->version.version, int, label_error);
	s_packet_read(pkt, &p->version.nserv, int, label_error);
	int i;
	for(i = 0; i < p->version.nserv; ++i) {
		s_packet_read(pkt, &p->version.servs[i], int, label_error);
	}

	s_log("[Acceptor] Phase 1 <Prepare Request>, Topic:(%s), ID(%d.%d), Value(%d), Version(%d), MyVerison(%d)",
			s_string_data_p(p->topic), p->id.x, p->id.y, p->value,
			p->version.version, paxos->version.version
	);

	if(paxos->version.version != p->version.version) {
		s_log("[Acceptor] Phase 1 <Prepare Request>, BAD Version!");

		s_packet_start_write(pkt, 0)
		{
			s_packet_wint(S_PAXOS_RESPONSE_BAD_VERSION);
		}
		s_packet_end_write();
		
		s_servg_rpc_ret(serv, req_id, pkt);

		s_packet_drop(pkt);

		s_paxos_proposal_drop(p);

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
		if(p->type != S_PAXOS_PROPOSAL_NORMAL) {
			int id = -1;
			while(1) {
				struct s_hash_key key;
				struct s_array ** tmp = s_hash_next(paxos->accept, &id, &key);
				if(!tmp) {
					break;
				}
				struct s_array * accept = *tmp;
				s_packet_wstring(key.str);		// write topic
				s_packet_wint(s_array_len(accept));	// value count
				int k;
				for(k = 0; k < s_array_len(accept); ++k) {
					struct s_paxos_accept * a = s_array_at(accept, k);
					s_packet_wint(a->value);
					s_packet_wint(a->id.x);
					s_packet_wint(a->id.y);
				}
			}
		}
	}
	s_packet_end_write();

	s_servg_rpc_ret(serv, req_id, pkt);

	s_packet_drop(pkt);

	// save response version
	if(p->type != S_PAXOS_PROPOSAL_NORMAL) {
		paxos->response_version = iget_new_version(p->topic);
		s_log("[Acceptor] Phase 1 <Prepare Request>, Save Response Version:%d", paxos->response_version);
	}

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

static void i_add_new_serv_ok(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_paxos_proposal * p = ud;
	struct s_core * core = p->core;
	struct s_paxos * paxos = core->paxos;

	// send accept to all servs
	s_log("[Proposal:Add New Member] Phase 2 <Accept Request>, Topic:(%s), ID:(%d.%d), Value:(%d)",
			s_string_data_p(p->topic), p->id.x, p->id.y, p->value);

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
			s_log("[Warning] s_paxos_response:send accept to serv:%d, missing!", serv_id);
			continue;
		}
		s_servg_send(serv, pkt);
	}

	s_packet_drop(pkt);
}

static void i_receive_response_add(struct s_core * core, struct s_packet * pkt, struct s_paxos_proposal * p, int serv_from)
{
	p->response.count++;
	struct s_string * topic = NULL;

	s_log("[Proposal:Add New Member] Phase 2 <Receive Prepare Response> from :%d", serv_from);

	if(pkt) {
		int suc;
		struct s_id max_id;
		int value;
		s_packet_read(pkt, &suc, int, label_error);
		if(suc == S_PAXOS_RESPONSE_BAD_VERSION) {
			s_log("[Proposal:Add New Member] Phase 2 <Receive Prepare Response> : Bad Version");
			p->response.count--;
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
		}
		// get all accept
		while(s_packet_read_string(pkt, &topic) >= 0) {
			int count;
			s_packet_read(pkt, &count, int, label_error);
			int i;
			for(i = 0; i < count; ++i) {
				int value;
				struct s_id id;
				s_packet_read(pkt, &value, int, label_error);
				s_packet_read(pkt, &id.x, int, label_error);
				s_packet_read(pkt, &id.y, int, label_error);
				ilearn(core, topic, value, &id, serv_from);
			}
			s_string_drop(topic);
			topic = NULL;
		}
	}

	if(p->response.count == p->version.nserv) {
		if(p->response.value > 0 && p->response.value != p->value) {
			s_log("[Proposal:Add New Member] Phase 2 <Receive Prepare Response> : conflict(%x - %x)!",
					p->response.value, p->value);
			return;
		}
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, p->the_serv);
		if(!serv) {
			s_log("[Warning:Add New Member] receive response add, miss serv:%d", p->the_serv);
			return;
		}
		struct s_packet * pkt = idump_learn(core, 1);
		s_packet_set_fun(pkt, S_PKT_TYPE_PAXOS_ADD_ACCEPT);
		s_servg_rpc_call(serv, pkt, s_paxos_proposal_grab(p), &i_add_new_serv_ok, 1000);

		s_packet_drop(pkt);
	}

label_error:
	if(topic) {
		s_string_drop(topic);
	}
	s_paxos_proposal_drop(p);
}

static void i_receive_response_rm(struct s_core * core, struct s_packet * pkt, struct s_paxos_proposal * p)
{
}

static void ireceive_response(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_paxos_proposal * p = ud;
	struct s_core * core = p->core;
	struct s_paxos * paxos = core->paxos;
	s_used(paxos);

	if(p->type == S_PAXOS_PROPOSAL_ADD) {
		i_receive_response_add(core, pkt, p, s_servg_get_id(serv));
		return;
	}
	if(p->type == S_PAXOS_PROPOSAL_RM) {
		i_receive_response_rm(core, pkt, p);
		return;
	}

	if(!pkt) {
		s_log("[Proposal] Phase 2 <Receive Prepare Response> : Timeout!");
		s_paxos_proposal_drop(p);
		return;
	}

	int suc;
	struct s_id max_id;
	int value;

	s_packet_read(pkt, &suc, int, label_error);
	if(suc == S_PAXOS_RESPONSE_BAD_VERSION) {
		s_log("[Proposal] Phase 2 <Receive Prepare Response> : bad version(id(%d.%d), topic:%s)!",
				p->id.x, p->id.y, s_string_data_p(p->topic));
		s_paxos_proposal_drop(p);
		return;
	}

	s_log("[Proposal] Phase 2 <Receive Prepare Response> From : %d", s_servg_get_id(serv));

	if(suc == S_PAXOS_RESPONSE_VALUE) {
		s_packet_read(pkt, &max_id.x, int, label_error);
		s_packet_read(pkt, &max_id.y, int, label_error);
		s_packet_read(pkt, &value, int, label_error);

		iresponse(core, p, &max_id, value);
	} else {
		iresponse(core, p, NULL, -1);
	}

label_error:

	s_paxos_proposal_drop(p);
	return;
}

static void iresponse(struct s_core * core, struct s_paxos_proposal * p, struct s_id * id, int value)
{
	struct s_paxos * paxos = core->paxos;

	p->response.count++;

	if(id) {
		s_log("[Proposal] Phase 2 <Receive Prepare Response> :ID(%d.%d), Value:%d", id->x, id->y, value);
		if(s_id_gt(id, &p->response.max_id)) {
			p->response.max_id = *id;
			p->response.value = value;
			p->value = value;
		}
	}

	if(p->response.count > p->version.nserv / 2) {
		s_log("[Proposal] Phase 2 <Accept Request> Begin: MaxID(%d.%d), Value:%d",
				p->response.max_id.x, p->response.max_id.y,
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
				s_log("[Warning] s_paxos_response:send accept to serv:%d, missing!", serv_id);
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

	struct s_string * topic = NULL;
	struct s_id id;
	int value;
	int version;

	s_packet_read(pkt, &topic, string, label_error);
	s_packet_read(pkt, &id.x, int, label_error);
	s_packet_read(pkt, &id.y, int, label_error);
	s_packet_read(pkt, &value, int, label_error);
	s_packet_read(pkt, &version, int, label_error);

	s_log("[Acceptor] Phase 2 <Accept Request>, Topic(%s), ID(%d.%d), Value(%d), Version(%d)",
			s_string_data_p(topic), id.x, id.y, value, version);

	iaccept(core, topic, &id, value, version);

label_error:
	if(topic) {
		s_string_drop(topic);
	}
	return;
}

// if version <= 0, ignore version
static void iaccept(struct s_core * core, struct s_string * topic, struct s_id * id, int value, int version)
{
	struct s_paxos * paxos = core->paxos;
	int v = iget_new_version(topic);

	if(version > 0 && version != paxos->version.version) {
		s_log("[Acceptor] Phase 2 <Accept Request> : Invalid Version(%d != %d)", version, paxos->version.version);
		return;
	}
	if(version > 0 && (version < paxos->response_version && v < paxos->response_version)) {
		s_log("[Acceptor] Phase 2 <Accept Request> : Version Refused (%d < response version:%d)",
				version, paxos->response_version);
		return;
	}

	struct s_id * pid = s_hash_get_str(paxos->response, topic);
	if(pid) {
		if(s_id_gt(pid, id)) {
			s_log("[Acceptor] Phase 2 <Accept Request> : ID Refused((%d.%d) < (%d.%d))",
					id->x, id->y, pid->x, pid->y);
			return;
		}
	}

	struct s_array ** pp = s_hash_get_str(paxos->accept, topic);
	if(!pp) {
		pp = s_hash_set_str(paxos->accept, topic);
		*pp = s_array_create(sizeof(struct s_paxos_accept), 16);
	}

	struct s_array * tmp = *pp;

	int i;
	for(i = 0; i < s_array_len(tmp); ++i) {
		struct s_paxos_accept * accept = s_array_at(tmp, i);
		if(s_string_equal(accept->topic, topic) && accept->value == value) {
			// already accept one
			if(s_id_gt(id, &accept->id)) {
				accept->id = *id;
			}
			break;
		}
	}

	if(i >= s_array_len(tmp)) {
		struct s_paxos_accept * accept = s_array_push(tmp);

		accept->topic = s_string_grab(topic);
		accept->id = *id;
		accept->value = value;
	}

	return;
}

static void ireceive_learn_request(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = ud;
	struct s_paxos * paxos = core->paxos;

	unsigned int req_id = s_packet_get_req(pkt);

	s_log("[Acceptor:For Learner] Receive Learn Request From :%d", s_servg_get_id(serv));

	s_packet_start_write(pkt, 0);
	{
	// start write packet

	struct s_hash_key key;
	int id = -1;
	while(1) {
		struct s_array ** pp = s_hash_next(paxos->accept, &id, &key);
		if(!pp) {
			break;
		}
		struct s_array * array = *pp;
		int i;
		for(i = 0; i < s_array_len(array); ++i) {
			struct s_paxos_accept * accept = s_array_at(array, i);
			s_packet_wstring(accept->topic);
			s_packet_wint(accept->value);
			s_packet_wint(accept->id.x);
			s_packet_wint(accept->id.y);
		}
	}

	// end write packet
	}
	s_packet_end_write();

	s_servg_rpc_ret(serv, req_id, pkt);

	s_packet_drop(pkt);
}

static void ireceive_learn(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = ud;
	struct s_paxos * paxos = core->paxos;

	paxos->learn_req.count--;

	struct s_string * topic = NULL;
	int value;
	struct s_id id;

	while(pkt && s_packet_read_string(pkt, &topic) >= 0) {
		s_packet_read(pkt, &value, int, label_error);
		s_packet_read(pkt, &id.x, int, label_error);
		s_packet_read(pkt, &id.y, int, label_error);

		ilearn(core, topic, value, &id, s_servg_get_id(serv));

		s_log("[Learner] Learn : Topic(%s) = Value(%d), From:%d",
				s_string_data_p(topic), value, s_servg_get_id(serv));

		s_string_drop(topic);
		topic = NULL;
	}

	if(paxos->learn_req.count == 0) {
		idump_learn(core, 0);
	}

label_error:
	if(topic) {
		s_string_drop(topic);
	}
	return;
}

static void ilearn(struct s_core * core, struct s_string * topic, int value, struct s_id * id, int serv_id)
{
	struct s_paxos * paxos = core->paxos;
	struct s_array ** pp = s_hash_get_str(paxos->learn, topic);
	if(!pp) {
		pp = s_hash_set_str(paxos->learn, topic);
		*pp = s_array_create(sizeof(struct s_paxos_learn), 16);
	}

	int i;
	for(i = 0; i < s_array_len(*pp); ++i) {
		struct s_paxos_learn * tmp = s_array_at(*pp, i);
		if(tmp->serv_id == serv_id && tmp->value == value) {
			if(s_id_gt(id, &tmp->id)) {
				tmp->id = *id;
			}
			return;
		}
	}

	struct s_paxos_learn * tmp = s_array_push(*pp);
	tmp->topic = s_string_grab(topic);
	tmp->value = value;
	tmp->serv_id = serv_id;
	tmp->id = *id;

	icheck_new_version(core);

	return;
}

static void icheck_new_version(struct s_core * core)
{
	struct s_paxos * paxos = core->paxos;
	struct s_hash_key key;
	int id = -1;
	while(1) {
		struct s_array ** pp = s_hash_next(paxos->learn, &id, &key);
		if(!pp) {
			break;
		}
		struct s_string * topic = key.str;
		int v = iget_new_version(topic);
		if(v == paxos->version.version + 1) {
			struct s_hash * h = s_hash_create(sizeof(int), 16);
			struct s_array * array = *pp;
			int i;
			for(i = 0; i < s_array_len(array); ++i) {
				struct s_paxos_learn * learn = s_array_at(array, i);
				int value = learn->value;
				int * pcount = s_hash_get_num(h, value);
				if(!pcount) {
					pcount = s_hash_set_num(h, value);
					*pcount = 0;
				}
				(*pcount)++;
				if(*pcount > paxos->version.nserv / 2) {
					s_log("[Learner] Change New Version To :%d, %x", v, value);
					paxos->version.version = v;
					int k;
					int c = 0;
					for(k = 0; k < 32; ++k) {
						if(value & (1 << k)) {
							paxos->version.servs[c++] = k;
							s_log("[Learner] Serv :%d", k);
							if(!s_servg_get_active_serv(core->servg, S_SERV_TYPE_M, k)) {
								s_servg_connect(core->servg, S_SERV_TYPE_M, k,
										"127.0.0.1", S_SERV_M_DEFAULT_PORT + k);
							}
						}
					}
					paxos->version.nserv = c;
					iaccept(core, topic, &learn->id, value, -1);
					break;
				}
			}
			s_hash_destroy(h);
			break;
		}
	}
}

static struct s_packet * idump_learn(struct s_core * core, int gen_pkt)
{
	struct s_paxos * paxos = core->paxos;
	struct s_packet * pkt = NULL;

	s_log("");
	s_log("======== Dump Begin =======");

	s_packet_start_write(pkt, 0)
	{
	// start write packet

	s_packet_wint(paxos->version.version);
	s_packet_wint(paxos->version.nserv);
	int i;
	for(i = 0; i < paxos->version.nserv; ++i) {
		s_packet_wint(paxos->version.servs[i]);
	}

	struct s_hash_key key;
	int id = -1;
	while(1) {
		struct s_array ** pp = s_hash_next(paxos->learn, &id, &key);
		if(!pp) {
			break;
		}
		struct s_string * topic = key.str;
		struct s_hash * h = s_hash_create(sizeof(int), 16);
		struct s_array * array = *pp;
		int i;
		for(i = 0; i < s_array_len(array); ++i) {
			struct s_paxos_learn * learn = s_array_at(array, i);
			if(!s_paxos_is_in(paxos, learn->serv_id)) {
				s_array_rm(array, i);
				i--;
				continue;
			}
			int value = learn->value;
			int * pcount = s_hash_get_num(h, value);
			if(!pcount) {
				pcount = s_hash_set_num(h, value);
				*pcount = 0;
			}
			(*pcount)++;
			if(*pcount > paxos->version.nserv / 2) {
				s_log("[LOG] Dump Learn %s : %d ( %x )", s_string_data_p(topic), value, value);
				
				s_packet_wstring(topic);
				s_packet_wint(value);
				s_packet_wint(learn->id.x);
				s_packet_wint(learn->id.y);
			}
		}
		s_hash_destroy(h);
	}

	// end write packet
	}
	s_packet_end_write();

	s_log("========");

	if(!gen_pkt) {
		s_packet_drop(pkt);
		return NULL;
	}
	return pkt;
}

static void ireceive_add(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = ud;
	struct s_paxos * paxos = core->paxos;

	int new_serv = s_servg_get_id(serv);

	struct s_string * topic = s_string_create_format("version:%d", paxos->version.version + 1);
	int value = 0;
	int i;
	for(i = 0; i < paxos->version.nserv; ++i) {
		value |= 1 << paxos->version.servs[i];
	}

	value |= 1 << new_serv;

	s_log("[Paxos] Add New Paxos Member. Start Paxos Topic:%s, Value:%x", s_string_data_p(topic), value);

	s_paxos_start(core, topic, value, S_PAXOS_PROPOSAL_ADD, new_serv);
}

static void ireceive_add_accept(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	s_log("[Paxos] I am New Member, Accept Others'");

	struct s_core * core = ud;
	struct s_paxos * paxos = core->paxos;
	struct s_string * topic = NULL;
	s_used(paxos);
	unsigned int req_id = s_packet_get_req(pkt);

	s_packet_read(pkt, &paxos->version.version, int, label_error);
	s_packet_read(pkt, &paxos->version.nserv, int, label_error);
	int i;
	for(i = 0; i < paxos->version.nserv; ++i) {
		s_packet_read(pkt, &paxos->version.servs[i], int, label_error);
	}

	while(s_packet_read_string(pkt, &topic) >= 0) {
		int value;
		struct s_id id;
		s_packet_read(pkt, &value, int, label_error);
		s_packet_read(pkt, &id.x, int, label_error);
		s_packet_read(pkt, &id.y, int, label_error);

		s_log("[Paxos:I Am New Member] Accept Others. Topic(%s), ID(%d.%d), Value(%d)",
				s_string_data_p(topic), id.x, id.y, value);

		iaccept(core, topic, &id, value, -1);

		s_string_drop(topic);
		topic = NULL;
	}

	pkt = s_packet_create(0);
	s_servg_rpc_ret(serv, req_id, pkt);
	s_packet_drop(pkt);

label_error:

	if(topic) {
		s_string_drop(topic);
	}
}


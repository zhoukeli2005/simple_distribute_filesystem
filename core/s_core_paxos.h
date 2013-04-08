#ifndef s_core_paxos_h_
#define s_core_paxos_h_

#include <s_common.h>
#include <s_array.h>
#include <s_string.h>
#include <s_hash.h>

#define S_PAXOS_MAX_SERV	16

struct s_core;

enum {
	S_PAXOS_RESPONSE_BAD_VERSION,
	S_PAXOS_RESPONSE_NO_VALUE,
	S_PAXOS_RESPONSE_VALUE
};

enum {
	S_PAXOS_PROPOSAL_ADD,
	S_PAXOS_PROPOSAL_RM,
	S_PAXOS_PROPOSAL_NORMAL
};

struct s_paxos_version {
	int version;
	
	int nserv;
	int servs[S_PAXOS_MAX_SERV];
};

struct s_paxos_accept {
	struct s_string * topic;

	struct s_id id;
	int value;
};

struct s_paxos_learn {
	struct s_string * topic;

	int value;
	int serv_id;
	struct s_id id;
};

struct s_paxos_proposal {
	struct s_core * core;
	int ref_count;

	struct s_paxos_version version;
	struct s_string * topic;
	int value;

	int type;
	int the_serv;

	struct s_id id;
	struct {
		int count;
		struct s_id max_id;
		int value;
	} response;
};

struct s_paxos {
	struct s_core * core;

	struct s_paxos_version version;

	int response_version;	// never accept/response to smaller version than this

	struct s_hash * my_proposal;

	struct s_hash * response;

	struct s_hash * accept;

	struct s_hash * learn;

	int id;

	struct {
		int count;
	} learn_req;
};

struct s_paxos *
s_paxos_create(struct s_core * core);

int
s_paxos_is_in(struct s_paxos * paxos, int serv);

void
s_paxos_start(struct s_core * core, struct s_string * topic, int value, int type, int the_serv);

void
s_paxos_learn_all(struct s_core * core);

//void
//s_paxos_response(struct s_core * core, struct s_paxos_proposal * p, struct s_id * id, int value);

#endif


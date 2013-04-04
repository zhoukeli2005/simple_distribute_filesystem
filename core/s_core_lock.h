#ifndef s_core_lock_h_
#define s_core_lock_h_

#include "s_core.h"

struct s_core_lock_elem {
	int serv_id;		// serv
	struct s_id id;		// data id

	int finish;
};

// data-struct in dserv
struct s_core_lock {
	struct s_list link;

	struct s_packet * pkt;

	struct s_id id;

	int client_id;

	unsigned int lock;

	int next_pos;
	int next_serv;
	int nserv;
	// servs
};

#define s_core_lock_create(n)	(struct s_core_lock *)s_malloc(char, sizeof(struct s_core_lock) + sizeof(struct s_core_lock_elem) * (n))
#define s_core_lock_p(c)	(struct s_core_lock_elem *)((struct s_core_lock *)(c) + 1)


#endif


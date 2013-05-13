#ifndef s_zookeeper_h_
#define s_zookeeper_h_

#include <s_common.h>
#include <s_mem.h>

#include <zookeeper.h>
#include <zookeeper.jute.h>

#define MAX_FILENAME_LEN	256

struct s_zoo {
	zhandle_t * zk;
	int lock_elem_id;
	struct s_hash * lock_elem_hash;
};

struct s_zoo *
s_zoo_init(const char * host);

/*
 *	lock
 *
 */
struct s_zoo_lock_elem;

typedef void(*lock_complete_t)(struct s_zoo * z, void * d, struct s_zoo_lock_elem * e);

struct s_zoo_lock_elem {
	struct s_zoo * z;

	lock_complete_t callback;
	void * d;

	char parent_path[MAX_FILENAME_LEN];
	char lock_path[MAX_FILENAME_LEN];

	int id;
};

void s_zoo_lock(struct s_zoo * z, const char * filename, lock_complete_t callback, void * d);
void s_zoo_unlock(struct s_zoo * z, struct s_zoo_lock_elem * e);

/*
 *	lock vector
 *
 */
struct s_zoo_lock_vector;

typedef void(*lockv_complete_t)(struct s_zoo * z, void * d, struct s_zoo_lock_vector * v);

struct s_zoo_lock_vector {
	struct s_zoo * z;

	int count;
	const char ** filenames;

	// internal
	int size;
	int curr;

	lockv_complete_t callback;
	void * d;

	struct s_zoo_lock_elem ** lock_elems;
};

struct s_zoo_lock_vector *
s_zoo_lockv_create(struct s_zoo * z);

void
s_zoo_lockv_add(struct s_zoo_lock_vector * v, const char * filename);

void
s_zoo_lockv(struct s_zoo * z, struct s_zoo_lock_vector * v, lockv_complete_t callback, void * d);

void
s_zoo_unlockv(struct s_zoo * z, struct s_zoo_lock_vector * v);

void
s_zoo_lockv_free(struct s_zoo_lock_vector * v);

/*
 *	Process Sync
 *
 */
typedef void(*s_sync_t)(struct s_zoo * z, void * d);
void
s_zoo_sync(struct s_zoo * z, int nprocess, const char * id);

#endif


#ifndef s_zookeeper_h_
#define s_zookeeper_h_

#include <s_common.h>
#include <s_mem.h>

#include <zookeeper.h>
#include <zookeeper.jute.h>

struct s_zoo {
	zhandle_t * zk;
};

struct s_zoo *
s_zoo_init(const char * host);

/*
 *	lock
 *
 */

typedef void(*lock_complete_t)(struct s_zoo * z, void * d, const char * lock_path);

void s_zoo_lock(struct s_zoo * z, const char * filename, lock_complete_t callback, void * d);
void s_zoo_unlock(struct s_zoo * z, const char * lock_path);

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

	const char ** lock_path;
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


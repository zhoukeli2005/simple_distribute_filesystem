#include <s_zookeeper.h>

struct thread_ctx {
	pthread_t thread;
	int id;
	struct s_zoo * z;
};

struct lock_ctx {
	int tid;
	int lid;
};

void lock_complete(struct s_zoo * z, void * d, struct s_zoo_lock_vector * v)
{
	struct lock_ctx * lc = d;
	s_log("get lockv, (%d,%d)", lc->tid, lc->lid);
	int i;
	for(i = 0; i < v->count; ++i) {
		s_log("%s, %s", v->filenames[i], v->lock_path[i]);
	}
	s_zoo_unlockv(z, v);
	s_zoo_lockv_free(v);
	s_log("unlockv, (%d, %d)", lc->tid, lc->lid);
}

static void sync_end(struct s_zoo * z, void * d)
{
	struct thread_ctx * ctx = d;
	s_log("sync end:%d", ctx->id);

}

void * lock_thread(void * d)
{
	struct thread_ctx * ctx = d;
	struct s_zoo * z = ctx->z;
	s_log("start sync:%d...", ctx->id);
	s_zoo_sync(z, 5, "sync1");
	struct timeval tv;
	gettimeofday(&tv, NULL);
	s_log("time:%ld s %ld us", tv.tv_sec, tv.tv_usec);
	/*

	int c = 0;

	while(1) {

		struct lock_ctx * lc = s_malloc(struct lock_ctx, 1);
		lc->tid = ctx->id;
		lc->lid = ++c;

		s_log("start lockv, (%d,%d) ... ", lc->tid, lc->lid);

		struct s_zoo_lock_vector * v = s_zoo_lockv_create(z);
		s_zoo_lockv_add(v, "lock1");
		s_zoo_lockv_add(v, "lock2");
		s_zoo_lockv_add(v, "lock3");

		s_zoo_lockv(z, v, &lock_complete, lc);

		break;
		sleep(5);
	}*/

	while(1) {
		sleep(1);
	}

	return NULL;
}

int main(int argc, char * argv[])
{
	struct s_zoo * z = s_zoo_init("127.0.0.1:2181");


	struct thread_ctx thread[5];
	int i;
	for(i = 0; i < 5; ++i) {
		struct thread_ctx * ctx = &thread[i];
		ctx->id = i + 1;
		ctx->z = z;
		pthread_create(&ctx->thread, NULL, &lock_thread, ctx);
	}

	for(i = 0; i < 5; ++i) {
		pthread_join(thread[i].thread, NULL);
	}

	return 0;
}

/*
#include <zookeeper.h>
#include <zookeeper.jute.h>
#include "zookeeper.h"
#include "zookeeper.jute.h"

struct context {
	zhandle_t * zookeeper;
};

static void print_event(int type, int state, const char * path);
static void print_error(int e);

static void zk_watcher(zhandle_t * zk, int type, int state, const char * path, void * ctx)
{
	print_event(type, state, path);

	struct context * c = (struct context *)ctx;
}

static void lock(const char * filename)
{
}

int main(int argc, char * argv[])
{
	struct context ctx;
	zhandle_t * zk = zookeeper_init("127.0.0.1:2181", &zk_watcher, 1000, NULL, &ctx, 0);
	ctx.zookeeper = zk;

	while(zoo_state(zk) != ZOO_CONNECTED_STATE) {
		// wait for session connected
	}

	// 1. create /lock
	{
		char path_buf[128];
		int r = zoo_create(zk, "/lock", NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, path_buf, 128);
		if(r != ZOK) {
			print_error(r);
		}

		
	}
	return 0;
}

static void print_event(int type, int state, const char * path)
{

#define CASE_TYPE(x)	if(type == x) 
#define CASE_STATE(x)	if(state == x)

	printf("event : type(%d), state(%d), path(%s):\t", type, state, path);
	CASE_TYPE(ZOO_CREATED_EVENT) {
		printf("create!");
	}

	CASE_TYPE(ZOO_DELETED_EVENT) {
		printf("deleted!");
	}

	CASE_TYPE(ZOO_CHANGED_EVENT) {
		printf("changed");
	}

	CASE_TYPE(ZOO_CHILD_EVENT) {
		printf("child");
	}

	CASE_TYPE(ZOO_SESSION_EVENT) {
		printf("session");
	}

	CASE_TYPE(ZOO_NOTWATCHING_EVENT) {
		printf("no watch");
	}

	printf("\t");

	CASE_STATE(ZOO_EXPIRED_SESSION_STATE) {
		printf("expired session");
	}

	CASE_STATE(ZOO_AUTH_FAILED_STATE) {
		printf("auth failed");
	}

	CASE_STATE(ZOO_CONNECTING_STATE) {
		printf("connecting");
	}

	CASE_STATE(ZOO_ASSOCIATING_STATE) {
		printf("associating");
	}

	CASE_STATE(ZOO_CONNECTED_STATE) {
		printf("coonnected");
	}
	printf("\n");
#undef CASE_TYPE
#undef CASE_STATE
}

static void print_error(int e)
{
#define CASE_PRINT(x)	\
	case x:	\
		printf("error:%s", #x);	\
	break

	switch(e) {
		CASE_PRINT(ZSYSTEMERROR);
		CASE_PRINT(ZRUNTIMEINCONSISTENCY);
		CASE_PRINT(ZDATAINCONSISTENCY);
		CASE_PRINT(ZCONNECTIONLOSS);
		CASE_PRINT(ZMARSHALLINGERROR);
		CASE_PRINT(ZUNIMPLEMENTED);
		CASE_PRINT(ZOPERATIONTIMEOUT);
		CASE_PRINT(ZBADARGUMENTS);
		CASE_PRINT(ZINVALIDSTATE);
		CASE_PRINT(ZAPIERROR);
		CASE_PRINT(ZNONODE);
		CASE_PRINT(ZNOAUTH);
		CASE_PRINT(ZBADVERSION);
		CASE_PRINT(ZNOCHILDRENFOREPHEMERALS);
		CASE_PRINT(ZNODEEXISTS);
		CASE_PRINT(ZNOTEMPTY);
		CASE_PRINT(ZSESSIONEXPIRED);
		CASE_PRINT(ZINVALIDCALLBACK);
		CASE_PRINT(ZINVALIDACL);
		CASE_PRINT(ZAUTHFAILED);
		CASE_PRINT(ZCLOSING);
		CASE_PRINT(ZNOTHING);
		CASE_PRINT(ZSESSIONMOVED);
		default:
			printf("unknown error:%d", e);
			break;
	}
	printf("\n");
#undef CASE_PRINT
}
*/

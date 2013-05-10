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

int main(int argc, char * argv[])
{
	struct context ctx;
	zhandle_t * zk = zookeeper_init("127.0.0.1:2181", &zk_watcher, 1000, NULL, &ctx, 0);
	ctx.zookeeper = zk;

	while(zoo_state(zk) != ZOO_CONNECTED_STATE) {
		// wait for session connected
	}

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

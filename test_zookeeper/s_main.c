#include <zookeeper.h>

struct context {
	zhandle_t * zookeeper;
};

static void zk_watcher(zhandle_t * zh, int type, int state, const char * path, void * ctx)
{

#define CASE_TYPE(x)	if(type == x) 
#define CASE_STATE(x)	if(state == x)

	printf("event : type(%d), state(%d), path(%p)\n", type, state, path);
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

	printf("\n");

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
}

int main(int argc, char * argv[])
{
	struct context ctx;
	zhandle_t * zk = zookeeper_init("127.0.0.1:2181", &zk_watcher, 1000, NULL, &ctx, 0);
	ctx.zookeeper = zk;

	while(1) {
		sleep(1);
	}

	return 0;
}

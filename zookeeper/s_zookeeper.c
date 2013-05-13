#include <s_zookeeper.h>

static void print_error(int e);
static void print_event(int type, int state, const char * path);

struct s_zoo g_zoo;
pthread_mutex_t lock_m;	// for s_zoo_lock
pthread_mutex_t lockv_m;// for s_zoo_lockv
pthread_mutex_t sync_m;// for s_zoo_sync

#define LOCK_ROOT	"/lock"
#define LOCK_ROOT_LEN	5
#define SYNC_ROOT	"/sync"
#define SYNC_ROOT_LEN	5

/*
struct s_zoo_lock_elem {
	struct s_zoo * z;
	void * d;

	char parent_path[MAX_FILENAME_LEN];
	char lock_path[MAX_FILENAME_LEN];

	lock_complete_t callback;
};*/

static int try_lock(struct s_zoo * z, const char * parent_path, const char * lock_path)
{
	int i;
	struct String_vector v;
	int r = zoo_get_children(z->zk, parent_path, 0, &v);
	if(r != ZOK) {
		print_error(r);
		return 0;
	}
	const char * min_str = 0;
	for(i = 0; i < v.count; ++i) {
		const char * str = v.data[i];
		if(!min_str || strcmp(min_str, str) < 0) {
			min_str = str;
		}
	}

	if(min_str && (strcmp(min_str, lock_path) == 0)) {
		return 1;
	}

	return 0;
}

static int try_sync(struct s_zoo * z, const char * parent_path, int nprocess)
{
	struct String_vector v;
	int r = zoo_get_children(z->zk, parent_path, 0, &v);
	if(r != ZOK) {
		print_error(r);
		return 0;
	}
	if(v.count >= nprocess) {
		return 1;
	}

	return 0;
}

static void lock_watcher(zhandle_t * zk, int type, int state, const char * path, void * ctx)
{
	print_event(type, state, path);

	struct s_zoo_lock_elem * elem = ctx;
	struct s_zoo * z = elem->z;
	if(try_lock(z, elem->parent_path, elem->lock_path)) {
		void * d = elem->d;
		lock_complete_t callback = elem->callback;
		callback(z, d, elem);
		return;
	}

	zoo_wexists(z->zk, elem->parent_path, &lock_watcher, elem, NULL);
}

struct s_zoo * s_zoo_init(const char * host)
{
	struct s_zoo * z = &g_zoo;
	z->zk = zookeeper_init(host, NULL, 1000, NULL, NULL, 0);
	while(zoo_state(z->zk) != ZOO_CONNECTED_STATE) {
		// wait for session connecting
	}

	// create '/lock'
	char path[MAX_FILENAME_LEN] = {0};
	int r = zoo_create(z->zk, LOCK_ROOT, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, path, sizeof(path));
	if(r != ZOK && r != ZNODEEXISTS) {
		print_error(r);
	}

	// create '/sync'
	r = zoo_create(z->zk, SYNC_ROOT, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, path, MAX_FILENAME_LEN);
	if(r != ZOK && r != ZNODEEXISTS) {
		print_error(r);
	}

	pthread_mutex_init(&lock_m, NULL);
	pthread_mutex_init(&lockv_m, NULL);
	pthread_mutex_init(&sync_m, NULL);

	return z;
}

void s_zoo_lock(struct s_zoo * z, const char * filename, lock_complete_t callback, void * d)
{
	static char path[MAX_FILENAME_LEN] = {0};
	static char path_created[MAX_FILENAME_LEN] = {0};

	pthread_mutex_lock(&lock_m);

	int fnlen = strlen(filename);
	int r;


	// 1. init static path "/lock/"
	{
		if(path[0] == 0) {
			memcpy(path, LOCK_ROOT, LOCK_ROOT_LEN);
		}
		path[LOCK_ROOT_LEN] = '/';
	}

	// 2. create node "/filename/"
	{
		if(fnlen + LOCK_ROOT_LEN + 1 >= MAX_FILENAME_LEN - 1) {
			printf("[Error]s_zoo_lock, filename(%s) too long(%d)!\n", filename, fnlen);
			goto label_end;
		}
		memcpy(path + LOCK_ROOT_LEN + 1, filename, fnlen + 1);

		r = zoo_create(z->zk, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, path_created, MAX_FILENAME_LEN);
		if(r != ZOK && r != ZNODEEXISTS) {
			printf("[Error]s_zoo_lock, create parent node:%s Error!\n", path);
			print_error(r);
			goto label_end;
		}
	}


	// 3. create node "/filename/seq"
	{
		path[LOCK_ROOT_LEN + 1 + fnlen] = '/';
		r = zoo_create(z->zk, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL|ZOO_SEQUENCE, path_created, MAX_FILENAME_LEN);
		if(r != ZOK) {
			printf("[Error]s_zoo_lock: create lock node:%s Error!\n", path);
			print_error(r);
			goto label_end;
		}

		const char * sep = strrchr(path_created, '/');
		sep++;	// skip '/'
		memmove(path_created, sep, strlen(sep) + 1);
	}

	// 4. check lock
	{
		path[LOCK_ROOT_LEN + 1 + fnlen] = 0;

		struct s_zoo_lock_elem * elem = s_malloc(struct s_zoo_lock_elem, 1);
		elem->z = z;
		elem->d = d;
		elem->callback = callback;
		memcpy(elem->parent_path, path, strlen(path) + 1);
		memcpy(elem->lock_path, path_created, strlen(path_created) + 1);

		if(try_lock(z, elem->parent_path, elem->lock_path)) {
			// lock ok
			pthread_mutex_unlock(&lock_m);

			callback(z, d, elem);
			return;
		}

		// not lock
		
		zoo_wexists(z->zk, elem->parent_path, &lock_watcher, elem, NULL);
	}

label_end:
	pthread_mutex_unlock(&lock_m);
}

void s_zoo_unlock(struct s_zoo * z, struct s_zoo_lock_elem * elem)
{
	zoo_delete(z->zk, elem->lock_path, -1);
	s_free(elem);
}

struct s_zoo_lock_vector * s_zoo_lockv_create(struct s_zoo * z)
{
	struct s_zoo_lock_vector * v = s_malloc(struct s_zoo_lock_vector, 1);
	v->z = z;
	v->count = 0;
	v->filenames = NULL;
	v->lock_elems = NULL;
	v->size = 0;
	v->curr = 0;

	return v;
}

void s_zoo_lockv_add(struct s_zoo_lock_vector * v, const char * filename)
{
	if(v->count >= v->size) {
		int newsize = (v->size + 1) * 2;
		const char ** old = v->filenames;
		v->filenames = s_malloc(const char *, newsize);
		int i;
		for(i = 0; i < v->count; ++i) {
			v->filenames[i] = old[i];
		}
		if(old) {
			s_free(old);
		}
		v->size = newsize;
	}
	v->filenames[v->count++] = strdup(filename);
}

void s_zoo_lockv_free(struct s_zoo_lock_vector * v)
{
	if(v->filenames) {
		int i;
		for(i = 0; i < v->count; ++i) {
			s_free((void*)(v->filenames[i]));
		}
		s_free(v->filenames);
	}
	if(v->lock_elems) {
		int i;
		for(i = 0; i < v->count; ++i) {
			if(v->lock_elems[i]) {
				s_free(v->lock_elems[i]);
			}
		}
		s_free(v->lock_elems);
	}
	s_free(v);
}

static void lockv_callback(struct s_zoo * z, void * d, struct s_zoo_lock_elem * elem)
{
//	printf("get lock:%s\n", file);

	struct s_zoo_lock_vector * v = d;

	v->lock_elems[v->curr++] = elem;
	if(v->curr >= v->count) {
		printf("[lockv]get all! callback!\n");

		v->callback(z, v->d, v);
		return;
	}
	s_zoo_lock(z, v->filenames[v->curr], &lockv_callback, v);
}

void s_zoo_lockv(struct s_zoo * z, struct s_zoo_lock_vector * v, lockv_complete_t callback, void * d)
{
	pthread_mutex_lock(&lockv_m);

	v->callback = callback;
	v->d = d;

	if(v->count <= 0) {
		printf("[Warning] s_zoo_lockv : count(%d) <= 0!\n", v->count);
		pthread_mutex_unlock(&lockv_m);

		callback(z, d, v);
		return;
	}

	v->lock_elems = s_malloc(struct s_zoo_lock_elem *, v->count);
	v->curr = 0;
	s_zoo_lock(z, v->filenames[0], &lockv_callback, v);

	pthread_mutex_unlock(&lockv_m);
}

void s_zoo_unlockv(struct s_zoo * z, struct s_zoo_lock_vector * v)
{
	int i;
	for(i = 0; i < v->count; ++i) {
		s_zoo_unlock(z, v->lock_elems[i]);
		v->lock_elems[i] = NULL;
	}
}

void s_zoo_sync(struct s_zoo * z, int nprocess, const char * id)
{
	static char path[MAX_FILENAME_LEN] = {0};
	static char path_created[MAX_FILENAME_LEN] = {0};

	pthread_mutex_lock(&sync_m);

	int fnlen = strlen(id);
	int r;


	// 1. init static path "/lock/"
	{
		if(path[0] == 0) {
			memcpy(path, SYNC_ROOT, SYNC_ROOT_LEN);
		}
		path[SYNC_ROOT_LEN] = '/';
	}

	// 2. create node "/id/"
	{
		if(fnlen + SYNC_ROOT_LEN + 1 >= MAX_FILENAME_LEN - 1) {
			printf("[Error]s_zoo_sync, id(%s) too long(%d)!\n", id, fnlen);
			goto label_end;
		}
		memcpy(path + SYNC_ROOT_LEN + 1, id, fnlen + 1);

		r = zoo_create(z->zk, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, path_created, MAX_FILENAME_LEN);
		if(r != ZOK && r != ZNODEEXISTS) {
			printf("[Error]s_zoo_sync, create parent node:%s Error!\n", path);
			print_error(r);
			goto label_end;
		}
	}


	// 3. create node "/id/seq"
	{
		path[SYNC_ROOT_LEN + 1 + fnlen] = '/';
		r = zoo_create(z->zk, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL|ZOO_SEQUENCE, path_created, MAX_FILENAME_LEN);
		if(r != ZOK) {
			printf("[Error]s_zoo_sync: create lock node:%s Error!\n", path);
			print_error(r);
			goto label_end;
		}

		const char * sep = strrchr(path_created, '/');
		sep++;	// skip '/'
		memmove(path_created, sep, strlen(sep) + 1);
	}

	// 4. check sync
	{
		path[SYNC_ROOT_LEN + 1 + fnlen] = 0;

		char * p = s_malloc(char, SYNC_ROOT_LEN + 1 + fnlen + 1);
		memcpy(p, path, SYNC_ROOT_LEN + 1 + fnlen + 1);

		pthread_mutex_unlock(&sync_m);

		while(!try_sync(z, p, nprocess)) {
			// sync not ok
			struct timespec tp = { .tv_nsec = 10 };
			nanosleep(&tp, NULL);
		}

		pthread_mutex_lock(&sync_m);
		s_free(p);
		pthread_mutex_unlock(&sync_m);

		return;
	}

label_end:
	pthread_mutex_unlock(&sync_m);
}

static void print_event(int type, int state, const char * path)
{

#define CASE_TYPE(x)    if(type == x) 
#define CASE_STATE(x)   if(state == x)

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
#define CASE_PRINT(x)   \
	case x: \
		printf("error:%s", #x); \
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


#include <s_thread.h>
#include <s_common.h>

#include <pthread.h>

struct s_thread {
	pthread_t ptid;

	S_THREAD_START_ROUTINE * start_routine;
	void * udata;
};

static struct s_thread * _get_free()
{
	static struct s_thread buf[MAX_THREAD];
	static int pos = 0;

	if(pos >= MAX_THREAD) {
		return NULL;
	}

	struct s_thread * t = &buf[pos++];
	return t;
}

static void * _default_start_routine(void * d)
{
	struct s_thread * t = (struct s_thread *)d;
	if(t->start_routine) {
		(*t->start_routine)(t);
	}
	return NULL;
}

struct s_thread *
s_thread_create(S_THREAD_START_ROUTINE * start_routine, void * data)
{
	struct s_thread * t = _get_free();
	if(!t) {
		return NULL;
	}

	t->udata = data;
	if(!pthread_create(&t->ptid, NULL, _default_start_routine, t)) {
		return NULL;
	}
	return t;
}

void*
s_thread_get_udata(struct s_thread * t)
{
	return t->udata;
}

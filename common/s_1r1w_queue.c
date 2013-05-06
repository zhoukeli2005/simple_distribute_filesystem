#include <s_1r1w_queue.h>
#include <s_mem.h>
#include <s_list.h>
#include <s_common.h>

struct s_1r1w_elem {
	struct s_1r1w_elem * prev;
	void * data;
};

#define S_1R1W_ELEM_POOL_SIZE	1024

struct s_1r1w_queue {
	struct s_1r1w_elem * head;
	struct s_1r1w_elem * tail;
	struct s_1r1w_elem * last;

	struct s_1r1w_elem elem_pool[S_1R1W_ELEM_POOL_SIZE];
	struct s_1r1w_elem * elem_free;
	pthread_mutex_t free_m;


	// stat
	int nmalloc;
	int nfree;
	int nget;
};

struct s_1r1w_queue * s_1r1w_queue_create( void )
{
	struct s_1r1w_queue * q = s_malloc(struct s_1r1w_queue, 1);
	q->head = q->tail = q->last = NULL;

	q->elem_free = NULL;
	int i;
	for(i = 0; i < S_1R1W_ELEM_POOL_SIZE; ++i) {
		struct s_1r1w_elem * e = &q->elem_pool[i];

		e->prev = q->elem_free;
		q->elem_free = e;
	}

	pthread_mutex_init(&q->free_m, NULL);

	// stat
	q->nmalloc = 0;
	q->nget = 0;
	q->nfree = 0;

	return q;
}

void s_1r1w_queue_destroy( struct s_1r1w_queue * q )
{
	// TODO: free elements
	
	s_free(q);
}

static struct s_1r1w_elem * iget_free_elem(struct s_1r1w_queue * q)
{
#define MAX_BUF 10000000
	static struct s_1r1w_elem g_buf[MAX_BUF];
	static int c = 0;
	return &g_buf[c++];

	/*
	struct s_1r1w_elem * elem = s_malloc(struct s_1r1w_elem, 1);
	elem->prev = elem->data = NULL;
	return elem;
	*/
	pthread_mutex_lock(&q->free_m);

	struct s_1r1w_elem * elem = NULL;
	if(q->elem_free) {
		q->nget++;
		elem = q->elem_free;
		q->elem_free = elem->prev;
	} else {
		q->nmalloc++;
		elem = s_malloc(struct s_1r1w_elem, 1);
	}

	pthread_mutex_unlock(&q->free_m);

	elem->prev = elem->data = NULL;


	return elem;
}

static void ifree_elem(struct s_1r1w_queue * q, struct s_1r1w_elem * elem)
{
	return;
//	s_free(elem);
	
	q->nfree++;

	pthread_mutex_lock(&q->free_m);

	elem->prev = q->elem_free;
	q->elem_free = elem;

	pthread_mutex_unlock(&q->free_m);
}

void s_1r1w_queue_push(struct s_1r1w_queue * q, void * d)
{
	struct s_1r1w_elem * elem = iget_free_elem(q);
	while(!elem) {
		elem = iget_free_elem(q);
	}
	elem->data = d;
	if(unlikely(!q->head)) {
		q->head = elem;
		q->tail = elem;
		return;
	}

	q->head->prev = elem;
	q->head = elem;

	return;
}

void * s_1r1w_queue_pop(struct s_1r1w_queue * q)
{
	if(!q->tail) {
		if(q->last) {
			q->tail = q->last->prev;
		}
		if(!q->tail) {
			return NULL;
		}
	}
	struct s_1r1w_elem * elem = q->tail;
	if(q->last) {
		ifree_elem(q, q->last);
	}
	q->last = elem;
	q->tail = elem->prev;

	return elem->data;
}

void s_1r1w_queue_dump_stat(struct s_1r1w_queue * q)
{
	s_log("[1r1wq] nmallc:%d, nget:%d, nfree:%d", q->nmalloc, q->nget, q->nfree);
}

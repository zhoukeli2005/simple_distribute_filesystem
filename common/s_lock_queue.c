#include <s_lock_queue.h>
#include <s_mem.h>
#include <s_list.h>

#include <s_common.h>

struct s_lockq_elem {
	struct s_list link;
	void * data;
};

#define S_LOCKQ_ELEM_POOL_SIZE	1024

struct s_lock_queue {
	pthread_mutex_t m;
	
	struct s_list elem_head;

	struct s_lockq_elem elem_pool[S_LOCKQ_ELEM_POOL_SIZE];
	struct s_list elem_free_head;
	pthread_mutex_t free_m;

	// stat
	int nmalloc;
	int nget;
	int nfree;
};

struct s_lock_queue *
s_lock_queue_create( void )
{
	struct s_lock_queue * q = s_malloc(struct s_lock_queue, 1);
	
	pthread_mutex_init(&q->m, NULL);

	s_list_init(&q->elem_head);

	s_list_init(&q->elem_free_head);
	int i;
	for(i = 0; i < S_LOCKQ_ELEM_POOL_SIZE; ++i) {
		struct s_lockq_elem * elem = &q->elem_pool[i];
		elem->data = NULL;
		s_list_insert(&q->elem_free_head, &elem->link);
	}

	pthread_mutex_init(&q->free_m, NULL);

	// stat
	q->nmalloc = q->nget = q->nfree = 0;

	return q;
}

void
s_lock_queue_destroy( struct s_lock_queue * q )
{
	// TODO : free elements

	s_free(q);
}

static struct s_lockq_elem * iget_free_elem(struct s_lock_queue * q)
{
	pthread_mutex_lock(&q->free_m);

	struct s_lockq_elem * elem;
	if(!s_list_empty(&q->elem_free_head)) {
		q->nget++;
		elem = s_list_first_entry(&q->elem_free_head, struct s_lockq_elem, link);
		s_list_del(&elem->link);
	} else {
		q->nmalloc++;
		elem = s_malloc(struct s_lockq_elem, 1);
	}

	s_list_init(&elem->link);

	pthread_mutex_unlock(&q->free_m);

	return elem;
}

static void ifree_elem(struct s_lock_queue * q, struct s_lockq_elem * elem)
{
	q->nfree++;
	pthread_mutex_lock(&q->free_m);

	s_list_insert(&q->elem_free_head, &elem->link);

	pthread_mutex_unlock(&q->free_m);
}

void s_lock_queue_push(struct s_lock_queue * q, void * d)
{
	pthread_mutex_lock(&q->m);

	struct s_lockq_elem * elem = iget_free_elem(q);

	elem->data = d;

	s_list_insert_tail(&q->elem_head, &elem->link);

	pthread_mutex_unlock(&q->m);
}

void * s_lock_queue_pop(struct s_lock_queue * q)
{
	pthread_mutex_lock(&q->m);

	void * data = NULL;
	if(!s_list_empty(&q->elem_head)) {
		struct s_lockq_elem * elem = s_list_first_entry(&q->elem_head, struct s_lockq_elem, link);
		s_list_del(&elem->link);
		data = elem->data;
		ifree_elem(q, elem);
	}

	pthread_mutex_unlock(&q->m);

	return data;
}

void s_lock_queue_dump_stat(struct s_lock_queue * q)
{
	s_log("[lockq] nmalloc:%d, nget:%d, nfree:%d", q->nmalloc, q->nget, q->nfree);
}

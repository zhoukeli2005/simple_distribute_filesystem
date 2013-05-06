#include <s_ms_queue.h>
#include <s_common.h>
#include <s_mem.h>

//#define USE_DEF_CAS

#ifdef USE_GCC_CAS

#define CAS __sync_bool_compare_and_swap 


#elif defined USE_DEF_CAS

#define CAS(_ptr, old, new) 							\
({										\
 	struct ms_pointer * __ptr = _ptr;					\
	struct ms_pointer __old = (old);					\
	struct ms_pointer __new = (new);					\
	int __ret;								\
	asm volatile("lock cmpxchg16b %1; sete %0"				\
		:"=m"(__ret),"+m"(*(volatile struct ms_pointer *)__ptr)		\
		:"a"(__old.ptr),"d"(__old.tag),"b"(__new.ptr),"c"(__new.tag)	\
		:"memory");							\
	__ret;									\
})

#else

static inline int CAS(struct ms_pointer * ptr, struct ms_pointer old, struct ms_pointer new)
{
	int ret;
	if(sizeof(*ptr) == 8) {
		asm volatile("lock cmpxchg8b %1; sete %0"
			:"=m"(ret), "+m"(*(volatile struct ms_pointer *)ptr)
			:"a"(old.ptr), "d"(old.tag), "b"(new.ptr), "c"(new.tag)
			:"memory");
		return ret;
	}

	if(sizeof(*ptr) == 16) {
		asm volatile("lock cmpxchg16b %1; sete %0"
			:"=m"(ret), "+m"(*(volatile struct ms_pointer *)ptr)
			:"a"(old.ptr), "d"(old.tag), "b"(new.ptr), "c"(new.tag)
			:"memory");
		return ret;
	}

	s_log("CAS Error! Wrong size:%lu", sizeof(*ptr));
	return -1;
}

#endif

#define ms_equal_pointer(p1, p2)	\
	(((p1)->ptr == (p2)->ptr) && ((p1)->tag == (p2)->tag))

#define ms_zero_pointer(p)	\
	(p)->ptr = NULL;	\
	(p)->tag = 0

#define ms_set_pointer(p, _ptr, _tag)	\
	(p)->ptr = _ptr;	\
	(p)->tag = _tag

static struct ms_node * _malloc_sz_16_(int sz)
{
	// 16 bytes aligned
	unsigned char * p = malloc(sz + 16);
	unsigned char * q = (unsigned char *)(((size_t)p + 0xF) & (~0xF));
	if(p == q) {
		q = p + 16;
	}
	unsigned char * x = q - 1;
	*x = q - p;
	return (struct ms_node *)q;
}

//#define _new_node()	_malloc_sz_16_(sizeof(struct ms_node))
static struct ms_node * _new_node()
{
#define MAX_BUF	10000000
	static char * g_buf = NULL;
	static size_t node_sz = 0;
	static int c = 0;
	if(!g_buf) {
		node_sz = sizeof(struct ms_node);
		node_sz = (node_sz + 0xF) & ~0xF;
		int sz = node_sz * MAX_BUF + 16;
		g_buf = malloc(sz);
		g_buf = ((size_t)g_buf + 0xF) & ~0xF;
	}
	char * p = g_buf + c * node_sz;
	c++;
	return (struct ms_node *)p;
}

static void _free_node(struct ms_node * n)
{
	return;
	unsigned char * q = (unsigned char *)n;
	unsigned char * x = q - 1;
	unsigned char * p = q - (*x);
	free(p);
}

struct ms_queue * ms_queue_create()
{
	struct ms_queue * q = (struct ms_queue *)_malloc_sz_16_(sizeof(struct ms_queue));

	struct ms_node * n = _new_node();
	n->ival = 0;
	ms_zero_pointer(&n->next);

	ms_set_pointer(&q->head, n, 0);
	ms_set_pointer(&q->tail, n, 0);

	return q;
}

void ms_queue_free(struct ms_queue * q)
{
	s_free(q);
}

void ms_queue_push(struct ms_queue * q, int v)
{
	struct ms_node * n = _new_node();
	n->ival = v;
	ms_zero_pointer(&n->next);

	struct ms_pointer tail, next, tmp;

	while(1) {
		tail = q->tail;
		next = tail.ptr->next;
		if(ms_equal_pointer(&tail, &q->tail)) {
			if(next.ptr == NULL) {
				ms_set_pointer(&tmp, n, next.tag+1);
				if(CAS(&(tail.ptr->next), next, tmp)) {
					break;
				}
			}else {
				ms_set_pointer(&tmp, next.ptr, tail.tag+1);
				CAS(&q->tail, tail, tmp);
			}
		}
	}
	ms_set_pointer(&tmp, n, tail.tag+1);
	CAS(&q->tail, tail, tmp);
}

int ms_queue_pop(struct ms_queue * q)
{
	int ret = -1;
	struct ms_pointer head, tail, next, tmp;
	while(1) {
		head = q->head;
		tail = q->tail;
		next = head.ptr->next;
		if(ms_equal_pointer(&head, &q->head)) {
			if(head.ptr == tail.ptr) {	// is empty
				if(next.ptr == NULL) {	// is empty
					return -1;
				}
				ms_set_pointer(&tmp, next.ptr, tail.tag+1);
				CAS(&q->tail, tail, tmp);
			} else {
				ret = next.ptr->ival;
				ms_set_pointer(&tmp, next.ptr, head.tag+1);
				if(CAS(&q->head, head, tmp)) {
					break;
				}
			}
		}
	}
	_free_node(head.ptr);
	return ret;
}


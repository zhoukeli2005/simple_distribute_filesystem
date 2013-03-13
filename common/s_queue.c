#include <s_queue.h>
#include <s_common.h>
#include <s_mem.h>

struct s_queue {
	int elem_size;

	int total;

	int first;
	int last;

	char * p;
};

#define iat(p, sz, n)	((char*)(p) + (sz) * (n))

struct s_queue * s_queue_create(int elem_size, int elem_num)
{
	elem_num = s_max(elem_num, 16);	
	struct s_queue * q = s_malloc(struct s_queue, 1);
	if(!q) {
		s_log("no mem for s_queue!");
		return NULL;
	}
	q->elem_size = elem_size;

	q->total = elem_num;

	q->first = q->last = 0;

	q->p = s_malloc(char, q->elem_size * q->total);
	if(!q->p) {
		s_log("no mem for s_queue!");
		s_free(q);
		return NULL;
	}

	return q;
}

void s_queue_destroy(struct s_queue * q)
{
	s_free(q->p);
	s_free(q);
}

void * s_queue_push(struct s_queue * q)
{
	if((q->last + 1) % q->total == q->first) {
		// full, expand to q->total * 2
		int sz = q->total * 2;
		char * p = s_malloc(char, q->elem_size * sz);
		if(!p) {
			s_log("no mem for s_queue.expand!");
			return NULL;
		}
		int i = 0;
		while(q->first != q->last) {
			void * src = iat(q->p, q->elem_size, q->first);
			void * dst = iat(p, q->elem_size, i++);
			memcpy(dst, src, q->elem_size);
			q->first = (q->first + 1) % q->total;
		}
		s_free(q->p);
		q->p = p;
		q->first = 0;
		q->last = i;
		q->total = sz;
	}
	void * p = iat(q->p, q->elem_size, q->last);
	q->last = (q->last + 1) % q->total;
	return p;
}

void * s_queue_pop(struct s_queue * q)
{
	if(s_queue_empty(q)) {
		return NULL;
	}

	void * p = s_queue_peek(q);
	q->first = (q->first + 1) % q->total;
	return p;
}

void * s_queue_peek(struct s_queue * q)
{
	if(s_queue_empty(q)) {
		return NULL;
	}
	return iat(q->p, q->elem_size, q->first);
}

int s_queue_empty(struct s_queue * q)
{
	if(q->last == q->first) {
		return 1;
	}
	return 0;
}

void s_queue_clear(struct s_queue * q)
{
	q->first = q->last = 0;
}

#if 0
int main(int argc, char * argv[])
{
	struct s_queue * q = s_queue_create(sizeof(int), 1);
	int i;
	int count = 0;
	srand(time(NULL));
	for(i = 0; i < 1000; ++i) {
		if(rand() % 4 != 1) {
			int * p = s_queue_push(q);
			*p = i;
			count++;
		} else {
			if(s_queue_pop(q)) {
				count--;
			}
		}
	}
	s_log("count:%d, total:%d, first:%d, last:%d\n", count, q->total, q->first, q->last);
	int * p;
	while(1) {
		p = s_queue_peek(q);
		if(!p) {
			break;
		}
		
		s_log("is:%d", *p);

		s_queue_pop(q);
	}
	return 0;
}
#endif

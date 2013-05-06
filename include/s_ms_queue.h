#ifndef s_ms_queue_h_
#define s_ms_queue_h_

/*	MS-queue

*/

struct ms_node;

struct ms_pointer {
	struct ms_node * ptr;
	unsigned int tag;
};

struct ms_node {
	struct ms_pointer next;
	union {
		void * pval;
		int ival;
		double fval;
	};
};

struct ms_queue {
	struct ms_pointer head;
	struct ms_pointer tail;
};

struct ms_queue *
ms_queue_create();

void
ms_queue_free(struct ms_queue * q);

void
ms_queue_push(struct ms_queue * q, int v);

int
ms_queue_pop(struct ms_queue * q);

#endif


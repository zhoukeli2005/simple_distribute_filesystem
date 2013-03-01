#ifndef s_queue_h_
#define s_queue_h_

/* first in first out */

struct s_queue;

struct s_queue *
s_queue_create(int elem_size, int elem_num);

void
s_queue_destroy(struct s_queue * queue);

void *
s_queue_push(struct s_queue * queue);

void *
s_queue_pop(struct s_queue * queue);

void *
s_queue_peek(struct s_queue * queue);

int
s_queue_empty(struct s_queue * queue);

void
s_queue_clear(struct s_queue * queue);

#endif


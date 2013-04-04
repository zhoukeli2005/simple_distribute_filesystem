#ifndef s_1r1w_queue_h_
#define s_1r1w_queue_h_

struct s_1r1w_queue;

struct s_1r1w_queue *
s_1r1w_queue_create( void );

void
s_1r1w_queue_destroy( struct s_1r1w_queue * q );

void
s_1r1w_queue_push(struct s_1r1w_queue * q, void * data);

void *
s_1r1w_queue_pop(struct s_1r1w_queue * q);

void
s_1r1w_queue_dump_stat(struct s_1r1w_queue * q);

#endif


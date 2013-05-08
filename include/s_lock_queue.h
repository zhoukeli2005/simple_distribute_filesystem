#ifndef s_lock_queue_h_
#define s_lock_queue_h_

struct s_lock_queue;

struct s_lock_queue *
s_lock_queue_create( int use_mutex );

void
s_lock_queue_destroy( struct s_lock_queue * q );

void
s_lock_queue_push( struct s_lock_queue * q, void * d );

void *
s_lock_queue_pop( struct s_lock_queue * q );

void
s_lock_queue_dump_stat( struct s_lock_queue * q );

#endif


#ifndef s_spinlock_queue_h_
#define s_spinlock_queue_h_

struct s_spinlock_queue;

struct s_spinlock_queue *
s_spinlock_queue_create( );

void
s_spinlock_queue_destroy( struct s_spinlock_queue * q );

void
s_spinlock_queue_push( struct s_spinlock_queue * q, void * d );

void *
s_spinlock_queue_pop( struct s_spinlock_queue * q );

void
s_spinlock_queue_dump_stat( struct s_spinlock_queue * q );

#endif


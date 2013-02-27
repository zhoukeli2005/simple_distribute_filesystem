#ifndef s_thread_h_
#define s_thread_h_

struct s_thread;

typedef void (* S_THREAD_START_ROUTINE)(struct s_thread * t);


/*
 *	create a thread
 *	@param start_routine : will be called when thread is created with param of struct s_thread
 *	@param data : will be stored in struct s_thread and can be fetched by s_thread_get_udata(...)
 *
 *	@return : the struct s_thread
 */
struct s_thread *
s_thread_create(S_THREAD_START_ROUTINE * start_routine, void * data);


/*
 *	retrieve the user data of struct s_thread that was passed in s_thread_create(...)
 *
 */
void *
s_thread_get_udata(struct s_thread * t);

#endif

#include <s_common.h>
#include <s_1r1w_queue.h>
#include <s_lock_queue.h>
#include <s_ms_queue.h>

#include <signal.h>

static int C;

#define LOOP	1000000

void * s_1r1wq_write(void * d)
{
	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct s_1r1w_queue * q = (struct s_1r1w_queue *)d;
	int i;
	for(i = 1; i <= LOOP; ++i) {
		s_1r1w_queue_push(q, (void *)i);
	}

	struct timeval end_tv;
	gettimeofday(&end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);
	s_log("[1r1wq write] time comsume : %ld s %ld us", tv.tv_sec, tv.tv_usec);

	return NULL;
}

void * s_1r1wq_read(void * d)
{
	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct s_1r1w_queue * q = (struct s_1r1w_queue *)d;

	int count = 0;
	int count_val = 0;

	long i = 0;
	while(i != LOOP) {
		void * d = s_1r1w_queue_pop(q);
		count++;
		if(!d) {
			continue;
		}
		long v = (long)d;
		if(v != (i + 1)) {
			s_log("[1r1wq] error!%ld!=%ld", v, i);
			exit(0);
		}
		i = v;
		count_val++;
	}

	struct timeval end_tv;
	gettimeofday(&end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);
	s_log("[1r1wq read] time comsume : %ld s %ld us, loop: %d , loop_v:%d", tv.tv_sec, tv.tv_usec, count, count_val);

	s_1r1w_queue_dump_stat(q);

	return NULL;
}

void * s_lockq_write(void * d)
{
	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct s_lock_queue * q = (struct s_lock_queue *)d;
	int i;
	for(i = 1; i <= LOOP; ++i) {
		s_lock_queue_push(q, (void *)i);
	}

	struct timeval end_tv;
	gettimeofday(&end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);
	s_log("[lockq write] time comsume : %ld s %ld us", tv.tv_sec, tv.tv_usec);

	return NULL;
}

void * s_lockq_read(void * d)
{
	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct s_lock_queue * q = (struct s_lock_queue *)d;

	int count = 0;
	int count_val = 0;

	long i = 0;
	while(i != LOOP) {
		void * d = s_lock_queue_pop(q);
		count++;
		if(!d) {
			continue;
		}
		long v = (long)d;
		if(v != i + 1) {
			s_log("[lockq] error!%ld!=%ld", v, i+1);
		}
		i = v;
		count_val++;
	}

	struct timeval end_tv;
	gettimeofday(&end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);
	s_log("[lockq read] time comsume : %ld s %ld us, loop: %d , loop_v:%d", tv.tv_sec, tv.tv_usec, count, count_val);

	s_lock_queue_dump_stat(q);

	return NULL;
}

void * s_msq_write(void * d)
{
	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct ms_queue * q = (struct ms_queue *)d;
	int i;
	for(i = 1; i <= LOOP; ++i) {
		ms_queue_push(q, i);
	}

	struct timeval end_tv;
	gettimeofday(&end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);
	s_log("[msq write] time comsume : %ld s %ld us", tv.tv_sec, tv.tv_usec);

	return NULL;
}

void * s_msq_read(void * d)
{
	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct ms_queue * q = (struct ms_queue *)d;

	int count = 0;
	int count_val = 0;

	long i = 0;
	while(i != LOOP) {
		int d = ms_queue_pop(q);
		count++;
		if(d <= 0) {
			continue;
		}
		long v = (long)d;
		if(v != i + 1) {
			s_log("[msq] error!%ld!=%ld", v, i+1);
		}
		i = v;
		count_val++;
	}

	struct timeval end_tv;
	gettimeofday(&end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);
	s_log("[msq read] time comsume : %ld s %ld us, loop: %d , loop_v:%d", tv.tv_sec, tv.tv_usec, count, count_val);

//	s_lock_queue_dump_stat(q);
	ms_queue_dump_stat(q);

	return NULL;
}

int main(int argc, char * argv[])
{
	if(argc == 2) {
		C = argv[1][0] - '0';
	}
	// Test One-Read-One-Write-NonLock-Queue
	if(C == 1)
	{
		struct s_1r1w_queue * q = s_1r1w_queue_create();

		pthread_t s_1r1wq_read_t, s_1r1wq_write_t;
		pthread_create(&s_1r1wq_write_t, NULL, &s_1r1wq_write, q);
		pthread_create(&s_1r1wq_read_t, NULL, &s_1r1wq_read, q);

		pthread_join(s_1r1wq_read_t, NULL);
		pthread_join(s_1r1wq_write_t, NULL);
	}


	// Test MS-Queue
	if(C == 2)
	{
		struct ms_queue * q = ms_queue_create();

		pthread_t s_msq_read_t, s_msq_write_t;
		pthread_create(&s_msq_write_t, NULL, &s_msq_write, q);
		pthread_create(&s_msq_read_t, NULL, &s_msq_read, q);

		pthread_join(s_msq_read_t, NULL);
		pthread_join(s_msq_write_t, NULL);
	}

	// Test Mutex Lock-Queue
	if(C == 3)
	{
		struct s_lock_queue * q = s_lock_queue_create(1);

		pthread_t s_lockq_read_t, s_lockq_write_t;
		pthread_create(&s_lockq_write_t, NULL, &s_lockq_write, q);
		pthread_create(&s_lockq_read_t, NULL, &s_lockq_read, q);

		pthread_join(s_lockq_read_t, NULL);
		pthread_join(s_lockq_write_t, NULL);
	}
	
	// Test Spin Lock-Queue
	if(C == 4)
	{
		struct s_lock_queue * q = s_lock_queue_create(0);

		pthread_t s_lockq_read_t, s_lockq_write_t;
		pthread_create(&s_lockq_write_t, NULL, &s_lockq_write, q);
		pthread_create(&s_lockq_read_t, NULL, &s_lockq_read, q);

		pthread_join(s_lockq_read_t, NULL);
		pthread_join(s_lockq_write_t, NULL);
	}

	return 0;
}


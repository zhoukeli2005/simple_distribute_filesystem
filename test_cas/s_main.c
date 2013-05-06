#include <s_common.h>
#include <s_ms_queue.h>

#include <signal.h>

#define LOOP	1000

void * s_lockq_write(void * d)
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

void * s_lockq_read(void * d)
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
			s_log("[lockq] error!%ld!=%ld", v, i+1);
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

	return NULL;
}

int main(int argc, char * argv[])
{
	// Test MS-Queue
	{
		struct ms_queue * q = ms_queue_create();

		pthread_t s_lockq_read_t, s_lockq_write_t;
		pthread_create(&s_lockq_write_t, NULL, &s_lockq_write, q);
		pthread_create(&s_lockq_read_t, NULL, &s_lockq_read, q);

		pthread_join(s_lockq_read_t, NULL);
		pthread_join(s_lockq_write_t, NULL);
	}


	return 0;
}


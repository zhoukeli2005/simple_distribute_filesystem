#include <s_common.h>
#include <s_1r1w_queue.h>
#include <s_lock_queue.h>
#include <s_spinlock_queue.h>
#include <s_ms_queue.h>

#include <signal.h>

static int SYN = 0;

#define TEST_3

// test 3
#ifdef TEST_3

#define WAIT_CYCLE	5

#define USE_WAIT
#define USE_SYNC

#endif	// -- end test 3

// test 2
#ifdef TEST_2

#endif	// -- end test 2

// test 1
#ifdef TEST_1

#define USE_SYNC

#endif	// -- end test 1

// use wait_a_while()
#ifdef USE_WAIT

#define wait_a_while()	\
do {	\
	int __i;	\
	for(__i = 0; __i < 10; ++__i) { }\
} while(0)

#else

#define wait_a_while()

#endif
// -- end

// use sync_thread()
#ifdef USE_SYNC

static void sync_thread(int nthread)
{
	volatile int * p = &SYN;
	asm volatile("lock addl $1, %0"
		:"+m"(*p)
		::"memory", "cc");
	for(;;) {
		if(*((volatile int *)(&SYN)) == nthread) {
			break;
		}
	}
}

#else

#define sync_thread(x)

#endif
// - end


static int C;

static int LOOP = 1000000;

void * s_1r1wq_write(void * d)
{
	sync_thread(2);

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
	sync_thread(2);

	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct s_1r1w_queue * q = (struct s_1r1w_queue *)d;

	int count = 0;
	int count_val = 0;
	int count_wait = 0;

	struct timespec tp = { 0 };

	long i = 0;
	while(i != LOOP) {

		void * d = s_1r1w_queue_pop(q);

		count++;

		if(!d) {
			wait_a_while();
			count_wait++;
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

	i = 0;
	while(i < count_wait) {
		wait_a_while();
		i++;
	}

	struct timeval the_end_tv;
	gettimeofday(&the_end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);

	struct timeval tmp;
	timersub(&the_end_tv, &end_tv, &tmp);

	struct timeval tt;
	timersub(&tv, &tmp, &tt);

	s_log("[1r1wq read] time comsume : %ld s %ld us, loop: %d , loop_v:%d", tt.tv_sec, tt.tv_usec, count, count_val);

	s_1r1w_queue_dump_stat(q);

	return NULL;
}

void * s_lockq_write(void * d)
{
	sync_thread(2);

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
	sync_thread(2);

	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct s_lock_queue * q = (struct s_lock_queue *)d;

	int count = 0;
	int count_val = 0;
	int count_wait = 0;

	long i = 0;
	while(i != LOOP) {
		wait_a_while();

		void * d = s_lock_queue_pop(q);
		count++;
		if(!d) {
			wait_a_while();
			count_wait++;
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

	i = 0;
	while(i < count_wait) {
		wait_a_while();
		i++;
	}

	struct timeval the_end_tv;
	gettimeofday(&the_end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);

	struct timeval tmp;
	timersub(&the_end_tv, &end_tv, &tmp);

	struct timeval tt;
	timersub(&tv, &tmp, &tt);

	s_log("[1r1wq read] time comsume : %ld s %ld us, loop: %d , loop_v:%d", tt.tv_sec, tt.tv_usec, count, count_val);

	s_lock_queue_dump_stat(q);

	return NULL;
}

void * s_slockq_write(void * d)
{
	sync_thread(2);
	printf("start write\n");

	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct s_spinlock_queue * q = (struct s_spinlock_queue *)d;
	int i;
	for(i = 1; i <= LOOP; ++i) {
		s_spinlock_queue_push(q, (void *)i);
	}

	struct timeval end_tv;
	gettimeofday(&end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);
	s_log("[lockq write] time comsume : %ld s %ld us", tv.tv_sec, tv.tv_usec);

	return NULL;
}

void * s_slockq_read(void * d)
{
	sync_thread(2);

	printf("start write\n");

	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct s_spinlock_queue * q = (struct s_spinlock_queue *)d;

	int count = 0;
	int count_val = 0;
	int count_wait = 0;

	long i = 0;
	while(i != LOOP) {
		wait_a_while();

		void * d = s_spinlock_queue_pop(q);
		count++;
		if(!d) {
			wait_a_while();
			count_wait++;
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

	i = 0;
	while(i < count_wait) {
		wait_a_while();
		i++;
	}

	struct timeval the_end_tv;
	gettimeofday(&the_end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);

	struct timeval tmp;
	timersub(&the_end_tv, &end_tv, &tmp);

	struct timeval tt;
	timersub(&tv, &tmp, &tt);

	s_log("[1r1wq read] time comsume : %ld s %ld us, loop: %d , loop_v:%d", tt.tv_sec, tt.tv_usec, count, count_val);
	/*
	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);
	s_log("[lockq read] time comsume : %ld s %ld us, loop: %d , loop_v:%d", tv.tv_sec, tv.tv_usec, count, count_val);
	*/

	return NULL;
}

void * s_msq_write(void * d)
{
	sync_thread(2);

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
	sync_thread(2);

	struct timeval start_tv;
	gettimeofday(&start_tv, NULL);

	struct ms_queue * q = (struct ms_queue *)d;

	int count = 0;
	int count_val = 0;
	int count_wait = 0;

	long i = 0;
	while(i != LOOP) {
		wait_a_while();

		int d = ms_queue_pop(q);
		count++;
		if(d <= 0) {
			wait_a_while();
			count_wait++;
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

	i = 0;
	while(i < count_wait) {
		wait_a_while();
		i++;
	}

	struct timeval the_end_tv;
	gettimeofday(&the_end_tv, NULL);

	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);

	struct timeval tmp;
	timersub(&the_end_tv, &end_tv, &tmp);

	struct timeval tt;
	timersub(&tv, &tmp, &tt);

	s_log("[1r1wq read] time comsume : %ld s %ld us, loop: %d , loop_v:%d", tt.tv_sec, tt.tv_usec, count, count_val);
	/*
	struct timeval tv;
	timersub(&end_tv, &start_tv, &tv);
	s_log("[msq read] time comsume : %ld s %ld us, loop: %d , loop_v:%d", tv.tv_sec, tv.tv_usec, count, count_val);

//	s_lock_queue_dump_stat(q);
	ms_queue_dump_stat(q);
	*/

	return NULL;
}

void test_3()
{
	
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
		struct s_spinlock_queue * q = s_spinlock_queue_create();

		pthread_t s_slockq_read_t, s_slockq_write_t;
		pthread_create(&s_slockq_write_t, NULL, &s_slockq_write, q);
		pthread_create(&s_slockq_read_t, NULL, &s_slockq_read, q);

		pthread_join(s_slockq_read_t, NULL);
		pthread_join(s_slockq_write_t, NULL);
	}
}

int main(int argc, char * argv[])
{
	if(argc >= 2) {
		C = argv[1][0] - '0';
	}
	if(argc >= 3) {
		LOOP = atoi(argv[2]) * 1000000;
	//	s_log("LOOP:%d", LOOP);
	}

#ifdef TEST_3
	test_3();
	return 0;
#endif
	
	// Test One-Read-One-Write-NonLock-Queue
	if(C == 1)
	{
		struct s_1r1w_queue * q = s_1r1w_queue_create();
#ifdef TEST_2
		s_1r1wq_write(q);
		s_1r1wq_read(q);

#else

		pthread_t s_1r1wq_read_t, s_1r1wq_write_t;
		pthread_create(&s_1r1wq_write_t, NULL, &s_1r1wq_write, q);
		pthread_create(&s_1r1wq_read_t, NULL, &s_1r1wq_read, q);

		pthread_join(s_1r1wq_read_t, NULL);
		pthread_join(s_1r1wq_write_t, NULL);
#endif
	}


	// Test MS-Queue
	if(C == 2)
	{
		struct ms_queue * q = ms_queue_create();
#ifdef TEST_2
		s_msq_write(q);
		s_msq_read(q);
#else

		pthread_t s_msq_read_t, s_msq_write_t;
		pthread_create(&s_msq_write_t, NULL, &s_msq_write, q);
		pthread_create(&s_msq_read_t, NULL, &s_msq_read, q);

		pthread_join(s_msq_read_t, NULL);
		pthread_join(s_msq_write_t, NULL);
#endif
	}

	// Test Mutex Lock-Queue
	if(C == 3)
	{
		struct s_lock_queue * q = s_lock_queue_create(1);
#ifdef TEST_2
		s_lockq_write(q);
		s_lockq_read(q);
#else

		pthread_t s_lockq_read_t, s_lockq_write_t;
		pthread_create(&s_lockq_write_t, NULL, &s_lockq_write, q);
		pthread_create(&s_lockq_read_t, NULL, &s_lockq_read, q);

		pthread_join(s_lockq_read_t, NULL);
		pthread_join(s_lockq_write_t, NULL);
#endif
	}
	
	// Test Spin Lock-Queue
	if(C == 4)
	{
		struct s_spinlock_queue * q = s_spinlock_queue_create();
#ifdef TEST_2
		s_slockq_write(q);
		s_slockq_read(q);
#else

		pthread_t s_slockq_read_t, s_slockq_write_t;
		pthread_create(&s_slockq_write_t, NULL, &s_slockq_write, q);
		pthread_create(&s_slockq_read_t, NULL, &s_slockq_read, q);

		pthread_join(s_slockq_read_t, NULL);
		pthread_join(s_slockq_write_t, NULL);
#endif
	}

	return 0;
}


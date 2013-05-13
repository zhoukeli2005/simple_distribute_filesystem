#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_DSERV	5
#define MAX_COUNT	1000

static int mycompare(const void * a, const void * b)
{
	const int * pa = a;
	const int * pb = b;
	return (*pa) - (*pb);
}

int main(int argc, char * argv[])
{
	int file = open("access_id.conf", O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);

	int servs[MAX_DSERV];
	int curr = 0;

	int servs_stat[MAX_DSERV] = {0};
	int count_stat[MAX_DSERV] = {0};

	const char * all_serv = "1,2,3,4,5\n";
	int all_serv_len = strlen(all_serv);

	int i;
	for(i = 0; i < MAX_COUNT; ++i) {
		int count = (rand() % MAX_DSERV) + 1;

		printf("i:%d, count:%d\n", i, count);
		count_stat[count-1]++;

		if(count == MAX_DSERV) {
			write(file, all_serv, all_serv_len);
			continue;
		}

		int k;
		int s;
		for(k = 0; k < count; ++k) {
label_regen:
			s = (rand() % MAX_DSERV) + 1;
			int n;
			for(n = 0; n < k; ++n) {
				if(servs[n] == s) {
					goto label_regen;
				}
			}

			printf("%d\t", s);

			servs[k] = s;
			servs_stat[s-1]++;
		}
		printf("\n");

		qsort(servs, count, sizeof(int), &mycompare);

		for(k = 0; k < count; ++k) {
			if(k > 0 && servs[k] == servs[k-1]) {
				printf("the same value:%d\n", servs[k]);
				return 0;
			}
			char buf[100];
			int n = snprintf(buf, 100, "%d", servs[k]);
			write(file, buf, n);
			if(k != count - 1) {
				write(file, ",", 1);
			}
		}

		write(file, "\n", 1);
	}

	close(file);

	for(i = 0; i < MAX_DSERV; ++i) {
		printf("count(%d):%d\n", i+1, count_stat[i]);
	}
	for(i = 0; i < MAX_DSERV; ++i) {
		printf("serv(%d):%d\n", i+1, servs_stat[i]);
	}

	return 0;
}

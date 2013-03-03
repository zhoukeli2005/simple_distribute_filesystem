#include <s_mem.h>
#include <s_string.h>
#include <s_file_reader.h>
#include <s_common.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

struct s_file_reader {
	char * p;
	int len;
};

struct s_file_reader * s_file_reader_create(const char * filename)
{
	int fd = open(filename, O_RDONLY);
	if(fd < 0) {
		s_log("open %s failed!", filename);
		return NULL;
	}

	struct stat s;
	if(fstat(fd, &s) < 0) {
		close(fd);
		s_log("fstat %s failed!", filename);
		return NULL;
	}

	int sz = s.st_size;
	if(sz <= 0) {
		close(fd);
		s_log("open empty file:%s!", filename);
		return NULL;
	}

	char * p = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);

	if(!p) {
		close(fd);
		s_log("mmap %s failed!", filename);
		return NULL;
	}

	struct s_file_reader * fr = s_malloc(struct s_file_reader, 1);
	if(!fr) {
		close(fd);
		munmap(p, sz);
		s_log("no mem for s_file_reader:%s!", filename);
		return NULL;
	}

	close(fd);

	fr->p = p;
	fr->len = sz;
	return fr;
}

void s_file_reader_destroy(struct s_file_reader * fr)
{
	munmap(fr->p, fr->len);
	s_free(fr);
}

char * s_file_reader_data_p(struct s_file_reader * fr)
{
	return fr->p;
}

int s_file_reader_data_len(struct s_file_reader * fr)
{
	return fr->len;
}

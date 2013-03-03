#include <s_common.h>
#include <s_string.h>
#include <s_mem.h>

struct s_string {
	unsigned int hash;
	unsigned int len;
	int ref_count;
};

#define gets(s)	(char *)(((struct s_string *)(s)) + 1)

unsigned int static inline ihash(const char * p, unsigned int len)
{
	unsigned int h = len;
	unsigned int step = (h >> 5) + 1;
	int i = h;
	for(i = h; i >= step; i -= step) {
		h = h ^ ((h<<5) + (h>>2) + (unsigned char)(p[i-1]));
	}
	return h;
}

struct s_string * s_string_create(const char * p)
{
	unsigned int len = strlen(p);
	return s_string_create_len(p, len);
}

struct s_string * s_string_create_len(const char * p, unsigned int len)
{
	struct s_string * str = (struct s_string *)s_malloc(char, sizeof(struct s_string) + len + 1);
	if(!str) {
		s_log("no mem for string!");
		return NULL;
	}
	str->len = len;
	str->hash = ihash(p, len);
	char * pdata = gets(str);
	memcpy(pdata, p, len);
	pdata[len] = 0;

	str->ref_count = 1;

	return str;
}

int s_string_equal(struct s_string * a, struct s_string * b)
{
	if(a == b) {
		return 1;
	}
	if(a->len == b->len && a->hash == b->hash &&
			strncmp(gets(a), gets(b), a->len) == 0) {
		return 1;
	}
	return 0;
}

const char * s_string_data_p(struct s_string * str)
{
	return gets(str);
}

int s_string_get_hash(struct s_string * str)
{
	return str->hash;
}

void s_string_get(struct s_string * str)
{
	str->ref_count++;
}

void s_string_put(struct s_string * str)
{
	str->ref_count--;
	if(str->ref_count == 0) {
		s_free(str);
		return;
	}
	if(str->ref_count < 0) {
		s_log("double free string!");
	}
	assert(str->ref_count > 0);
}

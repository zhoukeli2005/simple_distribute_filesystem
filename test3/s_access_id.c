#include "s_access_id.h"

#define MAX_LINE_LEN	256

struct read_contex {
	const char * p;
	int len;

	int curr;

	int line;
};

static struct s_array * read_line(struct read_contex * ctx)
{
	static char buf[MAX_LINE_LEN];

	if(ctx->curr >= ctx->len) {
		return NULL;
	}

	ctx->line++;

//	s_log("read line:%d", ctx->line);

	const char * p = ctx->p + ctx->curr;

	struct s_array * array = s_array_create(sizeof(int), 3);

	const char * q = p;
	while(*q != '\n') {
		q++;
	}

	int len = q - p;

	if(len >= MAX_LINE_LEN) {
		s_log("[Error] Line(%d) too long(%d)!", ctx->line, len);
		s_array_drop(array);
		return NULL;
	}
	memcpy(buf, p, len);
	buf[len] = 0;

	ctx->curr += len + 1;

	// read from buf
	const char * token;
	token = strtok(buf, ",");
	while(token) {
		int d = atoi(token);
		int * pp = s_array_push(array);
		*pp = d;

		token = strtok(NULL, ",");
	}

	return array;
}

static void dump(struct s_array * array)
{
	int Len = s_array_len(array);
	s_log("Len:%d", Len);
	int i;
	for(i = 0; i < Len; ++i) {
		struct s_array ** pp = s_array_at(array, i);
		struct s_array * p = *pp;
		s_log("Count:%d", s_array_len(p));
		int k;
		for(k = 0; k < s_array_len(p); ++k) {
			int * pi = s_array_at(p, k);
			int id = *pi;
			printf("\t%d,", id);
		}
		printf("\n");
	}
}

struct s_array * s_access_id_create(const char * conf)
{
	struct s_file_reader * fr = s_file_reader_create(conf);
	if(!fr) {
		s_log("[Error] no access id file:%s", conf);
		return NULL;
	}

	struct s_array * D = s_array_create(sizeof(struct s_array *), 16);

	const char * p = s_file_reader_data_p(fr);
	int len = s_file_reader_data_len(fr);

	struct read_contex ctx = {
		.p = p,
		.len = len,
		.curr = 0,
		.line = 0
	};

	struct s_array * array = read_line(&ctx);
	while(array) {
		struct s_array ** pp = s_array_push(D);
		*pp = array;

		array = read_line(&ctx);
	}

//	dump(D);

	s_file_reader_destroy(fr);

	return D;
}

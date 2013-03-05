#include <s_config.h>
#include <s_common.h>
#include <s_mem.h>
#include <s_file_reader.h>
#include <s_array.h>
#include <s_string.h>

#include <ctype.h>

/*
 *	mserver 1:
 *
 *		ip = 127.0.0.1
 *		port = 8888
 *
 *	mserver 2:
 *
 *		ip = 127.0.0.1
 *		port = 8889
 *
 */

#define DONTKNOWN	0
#define INT_VALUE	1
#define STR_VALUE	2

struct config_elem {
	struct s_string * key;
	union {
		int ival;
		struct s_string * sval;
	};
	int flags;
};

struct config_region {
	struct s_string * key;
	struct s_array * elems;
};

struct s_config {
	struct s_array * regions;
	struct config_region * curr_region;

	// for iter
	int iter_curr;
};

struct s_config * s_config_create(const char * filename)
{
#define error()	goto label_error
#define end_of_file() goto label_eof

	struct s_file_reader * fr = NULL;
	struct s_config * config = NULL;
	struct s_string * key = NULL, * val = NULL;

	fr = s_file_reader_create(filename);
	if(!fr) {
		s_log("config create failed:%s!", filename);
		error();
	}
	const char * p = s_file_reader_data_p(fr);
	int len = s_file_reader_data_len(fr);

	config = s_malloc(struct s_config, 1);
	if(!config) {
		s_log("no mem for config!");
		error();
	}
	config->curr_region = NULL;

	config->regions = s_array_create(sizeof(struct config_region), 16);
	if(!config->regions) {
		s_log("no mem for config->regions!");
		error();
	}

	// default region
	struct config_region * region = s_array_push(config->regions);
	if(!region) {
		s_log("no mem for default region!");
		error();
	}
	region->key = s_string_create("default");
	region->elems = s_array_create(sizeof(struct config_elem), 16);
	if(!region->elems) {
		s_log("no mem for region->elems!");
		error();
	}

#define skip_space()	\
	while(cur < len && isspace(p[cur])) cur++;	\
	if(cur >= len) {	\
		end_of_file();	\
	}

#define check_key_val()	\
	while(cur < len &&	\
		(!isspace(p[cur]) &&	\
		 p[cur] != ':' &&	\
		 p[cur] != '='	\
		)	\
	) cur++

#define next_line()	goto label_next_line


	int cur = 0;
label_next_line:
	// next line, skip space
	skip_space();

	// skip comment
	if(p[cur] == '#') {
		while((cur < len) && (p[cur] != '\r' && p[cur] != '\n')) cur++;
		next_line();
	}

	// key
	int key_start = cur;
	check_key_val();

	key = s_string_create_len(&p[key_start], cur - key_start);

	// skip space
	skip_space();

	// : or =
	char tmp = p[cur++];	// skip : or =
	if(tmp == ':') {
		// end last region, begin a new one
		region = s_array_push(config->regions);
		if(!region) {
			s_log("config:push new region failed!");
			error();
		}
		region->key = key;
		s_string_grab(key);

		region->elems = s_array_create(sizeof(struct config_elem), 16);
		if(!region->elems) {
			s_log("no mem for region->elems!");
			error();
		}

		next_line();
	}

	if(tmp != '=') {
		s_log("(cur:%d)expect ':' or '=', get:[%c]", cur, tmp);
		error();
	}

	// skip space
	skip_space();

	// val
	int val_start = cur;
	check_key_val();

	val = s_string_create_len(&p[val_start], cur - val_start);

	struct config_elem * elem = s_array_push(region->elems);
	if(!elem) {
		s_log("config:push new elem in region(%s) failed!", s_string_data_p(region->key));
		error();
	}
	elem->key = key;
	elem->sval = val;
	elem->flags = DONTKNOWN;

	s_string_grab(key);
	s_string_grab(val);

	next_line();

label_error:
	if(config) {
		s_config_destroy(config);
	}
	config = NULL;

label_eof:
	if(key) {
		s_string_drop(key);
	}
	if(val) {
		s_string_drop(val);
	}
	if(fr) {
		s_file_reader_destroy(fr);
	}

	return config;
}

void s_config_destroy(struct s_config * config)
{
	int i;
	for(i = 0; i < s_array_len(config->regions); ++i) {
		struct config_region * region = s_array_at(config->regions, i);
		s_string_drop(region->key);
		int k;
		for(k = 0; k < s_array_len(region->elems); ++k) {
			struct config_elem * elem = s_array_at(region->elems, k);
			s_string_drop(elem->key);
			if(elem->flags != INT_VALUE) {
				s_string_drop(elem->sval);
			}
		}
		s_array_drop(region->elems);
	}

	s_array_drop(config->regions);
}

int s_config_select_region(struct s_config * config, struct s_string * name)
{
	int i;
	for(i = 0; i < s_array_len(config->regions); ++i) {
		struct config_region * region = s_array_at(config->regions, i);
		if(s_string_equal(region->key, name)) {
			config->curr_region = region;
			return 1;
		}
	}
	return 0;
}

int s_config_read_int(struct s_config * config, struct s_string * key)
{
	int i;
	struct config_region * region = config->curr_region;
	if(!region) {
		s_log("no cur region!");
		return -1;
	}
	for(i = 0; i < s_array_len(region->elems); ++i) {
		struct config_elem * elem = s_array_at(region->elems, i);
		if(s_string_equal(elem->key, key)) {
			if(elem->flags == INT_VALUE) {
				return elem->ival;
			}
			if(elem->flags == DONTKNOWN) {
				elem->flags = INT_VALUE;
				elem->ival = atoi(s_string_data_p(elem->sval));
				return elem->ival;
			}
			s_log("error call! key(%s) is string value!", s_string_data_p(elem->key));
			return -1;
		}
	}
	s_log("no key in region(%s) key(%s)!", s_string_data_p(region->key), s_string_data_p(key));
	return -1;
}

struct s_string *
s_config_read_string(struct s_config * config, struct s_string * key)
{
	int i;
	struct config_region * region = config->curr_region;
	if(!region) {
		s_log("no cur region!");
		return NULL;
	}
	for(i = 0; i < s_array_len(region->elems); ++i) {
		struct config_elem * elem = s_array_at(region->elems, i);
		if(s_string_equal(elem->key, key)) {
			if(elem->flags == STR_VALUE) {
				return elem->sval;
			}
			if(elem->flags == DONTKNOWN) {
				elem->flags = STR_VALUE;
				return elem->sval;
			}
			s_log("error call! key(%s) is int value!", s_string_data_p(elem->key));
			return NULL;
		}
	}
	s_log("no key in region(%s) key(%s)!", s_string_data_p(region->key), s_string_data_p(key));
	return NULL;
}

void s_config_iter_begin(struct s_config * config)
{
	config->iter_curr = 0;
}

struct s_string * s_config_iter_next(struct s_config * config)
{
	if(config->iter_curr < 0 || config->iter_curr >= s_array_len(config->regions)) {
		return NULL;
	}

	struct config_region * region = s_array_at(config->regions, config->iter_curr++);
	return region->key;
}

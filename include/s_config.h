#ifndef s_config_h_
#define s_config_h_

#include <s_string.h>

struct s_config;

struct s_config *
s_config_create(const char * filename);

void
s_config_destroy(struct s_config * config);

int
s_config_select_region(struct s_config * c, struct s_string * name);

int
s_config_read_int(struct s_config * c, struct s_string * key);


/*
 *
 *	@return : weak reference, dont put!
 */
struct s_string *
s_config_read_string(struct s_config * c, struct s_string * key);



#define s_config_select(c, n)	({	\
		struct s_string * __n = s_string_create(n);	\
		int __r = s_config_select_region(c, __n);	\
		s_string_drop(__n);	\
		__r;})

#define s_config_read_i(c, k)	({	\
		struct s_string * __k = s_string_create(k);	\
		int __r = s_config_read_int(c, __k);	\
		s_string_drop(__k);	\
		__r;})

#define s_config_read_s(c, k)	({	\
		struct s_string * __k = s_string_create(k); 	\
		struct s_string * __r = s_config_read_string(c, __k);	\
		s_string_drop(__k);	\
		__r;})

/*
 *	iterate config region
 *
 */
void
s_config_iter_begin(struct s_config * config);

// @return : weak reference, dont `put`
struct s_string *
s_config_iter_next(struct s_config * config);

#endif


#ifndef s_string_h_
#define s_string_h_

struct s_string;

struct s_string *
s_string_create(const char * p);

struct s_string *
s_string_create_len(const char * p, unsigned int len);

const char *
s_string_data_p(struct s_string * str);

int
s_stirng_get_hash(struct s_string * str);

int
s_string_equal(struct s_string * a, struct s_string * b);

void
s_string_get(struct s_string * str);

void
s_string_put(struct s_string * str);


#endif


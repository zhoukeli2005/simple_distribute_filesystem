#ifndef s_string_h_
#define s_string_h_

struct s_string;

/*
 *	s_string_create
 *
 *	@explan : create string from const char * 
 *
 *	@param p : must be null-terminated string
 */
struct s_string *
s_string_create(const char * p);


/*
 *	s_string_create
 *
 *	@explan : create string from const char * with len
 */
struct s_string *
s_string_create_len(const char * p, unsigned int len);

/*
 *	s_string_create_format
 *
 *	@param format:
 *
 *		%d - int
 *		%s - string
 *
 *
 *	@notice : the len of result string is limited in 1024 bytes!
 */
#define S_STR_FORMAT_LIMIT	1024
struct s_string *
s_string_create_format(const char * format, ...);


/*
 *	s_string_data_p
 *
 *	@return : fetch raw const char * string
 */
const char *
s_string_data_p(struct s_string * str);

int
s_string_len(struct s_string * str);

int
s_stirng_get_hash(struct s_string * str);


int
s_string_equal(struct s_string * a, struct s_string * b);

void
s_string_grab(struct s_string * str);

void
s_string_drop(struct s_string * str);

#define s_string_assign(to, s)	s_string_grab(s); to = s


#endif


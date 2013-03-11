#ifndef s_hash_h_
#define s_hash_h_

#include <s_common.h>
#include <s_string.h>

struct s_hash;
struct s_hash_key;

#define S_HASH_MIN_NELEM_ALLOC	16

struct s_hash *
s_hash_create(int elem_size, int nelem_alloc);

void
s_hash_destroy(struct s_hash * hash);

void *
s_hash_set(struct s_hash * hash, struct s_hash_key * key);

void *
s_hash_get(struct s_hash * hash, struct s_hash_key * key);

void
s_hash_del(struct s_hash * hash, struct s_hash_key * key);

#define s_hash_set_str(hash, str) ({	\
		struct s_hash_key __key;	\
		__key.str = str;	\
		__key.tt = S_HASH_KEY_STR;	\
		void * __r = s_hash_set(hash, &__key);	\
		__r;	\
		})

#define s_hash_get_str(hash, str) ({	\
		struct s_hash_key __key;	\
		__key.str = str;	\
		__key.tt = S_HASH_KEY_STR;	\
		void * __r = s_hash_get(hash, &__key);	\
		__r;	\
		})

#define s_hash_del_str(hash, str) ({	\
		struct s_hash_key __key;	\
		__key.str = str;	\
		__key.tt = S_HASH_KEY_STR;	\
		void * __r = s_hash_del(hash, &__key);	\
		__r;	\
		})


#define s_hash_set_num(hash, num) ({	\
		struct s_hash_key __key;	\
		__key.num = num;	\
		__key.tt = S_HASH_KEY_NUM;	\
		void * __r = s_hash_set(hash, &__key);	\
		__r;	\
		})

#define s_hash_get_num(hash, num) ({	\
		struct s_hash_key __key;	\
		__key.num = num;	\
		__key.tt = S_HASH_KEY_NUM;	\
		void * __r = s_hash_get(hash, &__key);	\
		__r;	\
		})

#define s_hash_del_num(hash, num) ({	\
		struct s_hash_key __key;	\
		__key.num = num;	\
		__key.tt = S_HASH_KEY_NUM;	\
		void * __r = s_hash_del(hash, &__key);	\
		__r;	\
		})

#define s_hash_set_voidp(hash, voidp) ({	\
		struct s_hash_key __key;	\
		__key.p = voidp;	\
		__key.tt = S_HASH_KEY_VOIDP;	\
		void * __r = s_hash_set(hash, &__key);	\
		__r;	\
		})

#define s_hash_get_voidp(hash, voidp) ({	\
		struct s_hash_key __key;	\
		__key.p = voidp;	\
		__key.tt = S_HASH_KEY_VOIDP;	\
		void * __r = s_hash_get(hash, &__key);	\
		__r;	\
		})

#define s_hash_del_voidp(hash, voidp) ({	\
		struct s_hash_key __key;	\
		__key.p = voidp;	\
		__key.tt = S_HASH_KEY_VOIDP;	\
		void * __r = s_hash_del(hash, &__key);	\
		__r;	\
		})

enum S_E_HASH_KEY {
	S_HASH_KEY_STR = 1,
	S_HASH_KEY_NUM,
	S_HASH_KEY_VOIDP
};

struct s_hash_key {
	union {
		struct s_string * str;
		double num;
		void * p;
	};
	int tt;
};


#endif


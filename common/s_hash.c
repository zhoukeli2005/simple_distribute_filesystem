#include <s_hash.h>
#include <s_list.h>
#include <s_mem.h>
#include <s_string.h>

// node is unused
#define S_HASH_KEY_NULL	-1

struct s_hash_node {
	struct s_list link;
	struct s_hash_key key;
	// val follows
};

#define ival(n)		(char*)((struct s_hash_node *)(n) + 1)
#define inext(n)	(struct s_hash_node *)((n)->link.next)

struct s_hash {
	int elem_size;
	int node_size;

	int nelem_alloc;

	char * p;

	struct s_list free_node;
};

#define inode_at(p, sz, n)	(struct s_hash_node *)((char*)(p) + (sz) * (n))
#define ihash_at(h, n)		inode_at((h)->p, (h)->node_size, n)

static unsigned int iget_hash(struct s_hash_key * key)
{
	if(key->tt == S_HASH_KEY_STR) {
		struct s_string * str = key->str;
		return s_string_get_hash(str);
	}
	if(key->tt == S_HASH_KEY_NUM) {
		double d = key->num;
		if((double)(unsigned int)d == d) {
			return (unsigned int) d;
		}
		unsigned char * h = (unsigned char *)&d;
		int i;
		unsigned int hash = 0;
		for(i = 0; i < sizeof(double); ++i) {
			hash += h[i];
		}
		return hash;
	}
	if(key->tt == S_HASH_KEY_ID) {
		unsigned int hash = 0;
		hash = (unsigned int)(key->id.x + key->id.y);
		return hash;
	}
	if(key->tt != S_HASH_KEY_VOIDP) {
		s_log("[Error] key tt:%d!", key->tt);
	}
	assert(key->tt == S_HASH_KEY_VOIDP);
	union {
		void * p;
		unsigned int h;
	} u;
	u.p = key->p;
	return u.h;
}

static int iequal(struct s_hash_key * a, struct s_hash_key * b)
{
	if(a->tt != b->tt) {
		return 0;
	}
	if(a->tt == S_HASH_KEY_NUM) {
		return a->num == b->num;
	}
	if(a->tt == S_HASH_KEY_VOIDP) {
		return a->p == b->p;
	}
	if(a->tt == S_HASH_KEY_ID) {
		return (a->id.x == b->id.x) && (a->id.y == b->id.y);
	}
	return s_string_equal(a->str, b->str);
}

struct s_hash *
s_hash_create(int elem_size, int nelem_alloc)
{
	struct s_hash * hash = s_malloc(struct s_hash, 1);
	if(!hash) {
		s_log("[Error] no mem for s_hash!");
		return NULL;
	}
	nelem_alloc = s_max(nelem_alloc, S_HASH_MIN_NELEM_ALLOC);

	hash->elem_size = elem_size;
	hash->nelem_alloc = nelem_alloc;

	hash->node_size = sizeof(struct s_hash_node) + elem_size;

	int sz = hash->node_size * nelem_alloc;
	hash->p = s_malloc(char, sz);
	if(!hash->p) {
		s_log("[Error] no mem for s_hash.p!");
		s_free(hash);
		return NULL;
	}

	s_list_init(&hash->free_node);
	
	int i;
	for(i = 0; i < nelem_alloc; ++i) {
		struct s_hash_node * node = ihash_at(hash, i);
		s_list_insert(&hash->free_node, &node->link);
		node->key.tt = S_HASH_KEY_NULL;
	}

	return hash;
}

static struct s_hash_node *
i_get_free_node(struct s_hash * hash)
{
	if(!s_list_empty(&hash->free_node)) {
		struct s_hash_node * node = s_list_first_entry(&hash->free_node, struct s_hash_node, link);
		s_list_del(&node->link);
		return node;
	}

	// expand
	char * old_p = hash->p;
	int old_alloc = hash->nelem_alloc;
	int new_alloc = hash->nelem_alloc * 2;
	hash->p = s_malloc(char, hash->node_size * new_alloc);
	hash->nelem_alloc = new_alloc;

	if(!hash->p) {
		s_log("[Error] no mem for s_hash.expand!");
		return NULL;
	}

	s_list_init(&hash->free_node);
	int i;
	for(i = 0; i < hash->nelem_alloc; ++i) {
		struct s_hash_node * node = ihash_at(hash, i);
		node->key.tt = S_HASH_KEY_NULL;
		s_list_insert(&hash->free_node, &node->link);
	}
	for(i = 0; i < old_alloc; ++i) {
		struct s_hash_node * old = inode_at(old_p, hash->node_size, i);
		if(old->key.tt != S_HASH_KEY_NULL) {
			void * pval = s_hash_set(hash, &old->key);
			memcpy(pval, ival(old), hash->elem_size);
		} else {
			s_log("[Warnning] s_hash.expand while has free slot!");
		}
	}
	s_free(old_p);

	return i_get_free_node(hash);
}

void
s_hash_destroy(struct s_hash * hash)
{
	// free str
	int i;
	for(i = 0; i < hash->nelem_alloc; ++i) {
		struct s_hash_node * node = ihash_at(hash, i);
		if(node->key.tt == S_HASH_KEY_STR) {
			s_string_drop(node->key.str);
		}
	}

	s_free(hash->p);
	s_free(hash);
}

void * s_hash_set(struct s_hash * H, struct s_hash_key * key)
{
	// 0. if already has `key`
	void * find_p = s_hash_get(H, key);
	if(find_p) {
		return find_p;
	}

	struct s_hash_node * free_node = i_get_free_node(H);
	if(!free_node) {
		s_log("[Error] no mem for s_hash_set!");
		return NULL;
	}

	unsigned int hash = iget_hash(key);
	unsigned int main_pos = hash % H->nelem_alloc;
	struct s_hash_node * node = ihash_at(H, main_pos);

	// 0. if key is string, grab string 
	if(key->tt == S_HASH_KEY_STR) {
		s_string_grab(key->str);
	}

	// 1. empty node
	if(node->key.tt == S_HASH_KEY_NULL) {
		// need not `free_node`
		s_list_insert(&H->free_node, &free_node->link);

		// remove from free list
		s_list_del(&node->link);

		node->key = *key;
		node->link.next = NULL;
		return ival(node);
	}

	unsigned int the_hash = iget_hash(&node->key);
	unsigned int the_mp = the_hash % H->nelem_alloc;

	// 2. at main position
	if(main_pos == the_mp) {
		free_node->key = *key;
		free_node->link.next = node->link.next;
		node->link.next = (void*)free_node;
		return ival(free_node);
	}

	// 3. do not at main position, move it
	struct s_hash_node * the_node = ihash_at(H, the_mp);
	while(the_node->link.next && (void*)(the_node->link.next) != (void*)node) {
		the_node = inext(the_node);
	}

	assert(inext(the_node) == node);

	memcpy((void*)free_node, (void*)node, H->node_size);
	the_node->link.next = (void*)free_node;

	node->link.next = NULL;
	node->key = *key;
	return ival(node);
}

void * s_hash_get(struct s_hash * H, struct s_hash_key * key)
{
	unsigned int hash = iget_hash(key);
	unsigned int mp = hash % H->nelem_alloc;
	struct s_hash_node * node = ihash_at(H, mp);
	while(node && node->key.tt != S_HASH_KEY_NULL) {
		if(iequal(&node->key, key)) {
			// find
			return ival(node);
		}
		node = inext(node);
	}
	return NULL;
}

void s_hash_del(struct s_hash * H, struct s_hash_key * key)
{
	unsigned int hash = iget_hash(key);
	unsigned int mp = hash % H->nelem_alloc;
	struct s_hash_node * main_node = ihash_at(H, mp);

	struct s_hash_node * node = main_node;
	struct s_hash_node * prev = NULL;
	while(node && node->key.tt != S_HASH_KEY_NULL) {
		if(iequal(&node->key, key)) {
			// find

			// 1. if key is string, drop it
			if(node->key.tt == S_HASH_KEY_STR) {
				s_string_drop(node->key.str);
			}

			struct s_hash_node * to_be_free = NULL;

			// 2. if has prev
			if(prev) {
				prev->link.next = (void*)inext(node);
				to_be_free = node;
			} else {

			// 3. no prev

				struct s_hash_node * n = inext(node);

				if(n) {

				// 3.1 if has next

					memcpy(main_node, n, H->node_size);
					to_be_free = n;

				} else {

				// 3.2 no next

					to_be_free = main_node;
				}
			}
			to_be_free->key.tt = S_HASH_KEY_NULL;
			s_list_insert(&H->free_node, &to_be_free->link);
			return;
		}
		prev = node;
		node = inext(node);
	}
}

void * s_hash_next(struct s_hash * hash, int * id, struct s_hash_key * key)
{
	int i = s_max(0, *id);
	struct s_hash_node * node;
	for(; i < hash->nelem_alloc; ++i) {
		node = ihash_at(hash, i);
		if(node->key.tt != S_HASH_KEY_NULL) {
			*id = i + 1;
			if(key) {
				*key = node->key;
			}
			return ival(node);
		}
	}
	if(key) {
		key->tt = S_HASH_KEY_NULL ;
	}
	return NULL;
}

#if 0
int main(int argc, char * argv[])
{
	struct A {
		int i;
		char str[64];
	};

	srand(time(NULL));

	struct s_hash * hash = s_hash_create(sizeof(struct A), 0);
	int i;
	for(i = 0; i < 1600; ++i) {
		int r = rand() % 1000;
		if(r < 500) {
			struct A * p = s_hash_set_num(hash, r);
			p->i = i;
			snprintf(p->str, 63, "%d", r);
		} else {
			struct s_string * str = s_string_create_format("%d", r);
			struct A * p = s_hash_set_str(hash, str);
			p->i = i;
			memcpy(p->str, s_string_data_p(str), s_string_len(str) + 1);
		}
	}

	s_log("node_size:%d, elem_size:%d, nalloc:%d", hash->node_size, hash->elem_size, hash->nelem_alloc);

	struct A * p = s_hash_set_num(hash, 50.0);
	p->i = 50;
	memcpy(p->str, "heha", 5);

	p = s_hash_get_num(hash, 50);
	if(p) {
		s_log("find, %d, %s", p->i, p->str);
	} else {
		s_log("!!not find!");
	}

	p = s_hash_get_num(hash, 50.001);
	if(p) {
		s_log("!!find %f, %d, %s", 50.001, p->i, p->str);
	} else {
		s_log("not found:50.001");
	}

	p = s_hash_set_str(hash, s_string_create("50"));
	p->i = 50;
	memcpy(p->str, "xxxx", 5);

	p = s_hash_get_str(hash, s_string_create("50"));
	if(p) {
		s_log("find %d, %s", p->i, p->str);
	} else {
		s_log("!! not found!");
	}

	return 0;
}

#endif

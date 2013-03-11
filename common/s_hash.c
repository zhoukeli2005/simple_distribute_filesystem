#include <s_hash.h>
#include <s_list.h>
#include <s_mem.h>
#include <s_string.h>

// node is unused
#define S_HASH_KEY_NULL	-1

struct s_hash_node {
	struct s_list link;
	struct s_hash_key key;
	char val[1];
};

#define ival(n)	(&(n)->val)
#define inext(n)	((struct s_hash_node *)((n)->link.next))

struct s_hash {
	int elem_size;
	int node_size;

	int nelem_alloc;

	char * p;

	struct s_list free_node;
};

#define inode_at(p, sz, n)	(struct s_hash_node *)(((char*)(p)) + (sz) * (n))
#define ihash_at(h, n)	inode_at((h)->p, (h)->node_size, n)

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

	hash->node_size = sizeof(struct s_hash_node) + elem_size - 1;

	int sz = hash->node_size * nelem_alloc;
	hash->p = s_malloc(char, sz);

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
		struct s_hash_node * node = s_hash_set(hash, &old->key);
		memcpy(ival(node), ival(old), hash->elem_size);
	}

	return i_get_free_node(hash);
}

void
s_hash_destroy(struct s_hash * hash)
{
	// free str XXX

	s_free(hash->p);
	s_free(hash);
}

void * s_hash_set(struct s_hash * H, struct s_hash_key * key)
{
	unsigned int hash = iget_hash(key);
	unsigned int main_pos = hash % H->nelem_alloc;
	struct s_hash_node * node = ihash_at(H, main_pos);

	// 0. if key is string, grab string 
	if(key->tt == S_HASH_KEY_STR) {
		s_string_grab(key->str);
	}

	// 1. empty node
	if(node->key.tt == S_HASH_KEY_NULL) {
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
		struct s_hash_node * free_node = i_get_free_node(H);
		if(!free_node) {
			return NULL;
		}
		free_node->key = *key;
		free_node->link.next = node->link.next;
		node->link.next = (void*)free_node;
		return ival(node);
	}

	// 3. do not at main position, move it
	struct s_hash_node * the_node = ihash_at(H, the_mp);
	while(the_node->link.next && (void*)(the_node->link.next) != (void*)node) {
		the_node = inext(the_node);
	}

	assert(inext(the_node) == node);

	struct s_hash_node * free_node = i_get_free_node(H);
	if(!free_node) {
		return NULL;
	}
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


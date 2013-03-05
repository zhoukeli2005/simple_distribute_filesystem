#include <s_mem.h>
#include <s_common.h>
#include <s_array.h>

struct s_array {
	int ref_count;

	int elem_size;

	char * p;
	int nelem;	// mem used
	int total;	// mem allocated
};

#define iat(p, s, n)	(((char *)(p)) + (s) * (n))

struct s_array *
s_array_create(int elem_size, int elem_num)
{
	elem_num = s_max(elem_num, 16);

	struct s_array * array = s_malloc(struct s_array, 1);

	array->ref_count = 1;
	array->elem_size = elem_size;

	array->total = elem_num;
	array->nelem = 0;

	array->p = s_malloc(char, array->elem_size * array->total);
	if(!array->p) {
		s_free(array);
		s_log("no mem for array->p!");
		return NULL;
	}
	return array;
}

int s_array_len(struct s_array * array)
{
	return array->nelem;
}

static int expand(struct s_array * array, int sz)
{
	if(array->total >= sz) {
		return 0;
	}
	char * old = array->p;
	array->p = s_malloc(char, sz * array->elem_size);
	if(!array->p) {
		s_log("no mem for array expand(curr size:%d/%d, to :%d)", array->nelem, array->total, sz);
		return -1;
	}
	memcpy(array->p, old, array->nelem * array->elem_size);
	s_free(old);
	array->total = sz;
	return 0;
}

void *
s_array_push(struct s_array * array) 
{
	if(array->nelem >= array->total) {
		// expand
		int sz = array->total * 2;
		/*
		char * old = array->p;
		array->p = s_malloc(char, sz * array->elem_size);
		if(!array->p) {
			s_log("no mem for array expand(curr size:%d/%d)", array->nelem, array->total);
			return NULL;
		}
		memcpy(array->p, old, array->nelem * array->elem_size);
		s_free(old);
		array->total = sz;*/
		if(expand(array, sz) < 0) {
			return NULL;
		}
	}
	array->nelem++;
	return iat(array->p, array->elem_size, array->nelem-1);
}

void *
s_array_at(struct s_array * array, int index)
{
	if(index < 0 || index >= array->nelem) {
		s_log("invalid index:%d", index);
		return NULL;
	}

	return iat(array->p, array->elem_size, index);
}

void
s_array_rm(struct s_array * array, int index)
{
	if(index < 0 || index >= array->nelem) {
		s_log("invalid index:%d, nelem:%d", index, array->nelem);
		return;
	}

	int i;
	for(i = index; i < array->nelem - 1; ++i) {
		char * dst = iat(array->p, array->elem_size, i);
		char * src = iat(array->p, array->elem_size, i+1);
		memcpy(dst, src, array->elem_size);
	}

	array->nelem--;
}

void s_array_grab(struct s_array * array)
{
	array->ref_count++;
}

void s_array_drop(struct s_array * array)
{
	array->ref_count--;
	if(array->ref_count == 0) {
		s_free(array);
		return;
	}

	assert(array->ref_count > 0);
	if(array->ref_count < 0) {
		s_log("double free s_array!");
		return;
	}
}


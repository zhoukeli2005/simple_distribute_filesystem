#ifndef s_array_h_
#define s_array_h_

struct s_array;

struct s_array *
s_array_create(int elem_size, int elem_num);

int
s_array_len(struct s_array * array);

void *
s_array_push(struct s_array * array);

void *
s_array_at(struct s_array * array, int index);

void
s_array_rm(struct s_array * array, int index);

void
s_array_grab(struct s_array * array);

void
s_array_drop(struct s_array * array);


#endif


#ifndef s_list_h_
#define s_list_h_

#include "s_common.h"

struct s_list {
	struct s_list * prev;
	struct s_list * next;
};

static inline void s_list_init(struct s_list * s)
{
	s->prev = s->next = s;
}

static inline void _s_list_insert(struct s_list * s, struct s_list * prev, struct s_list * next)
{
	s->prev = prev;
	s->next = next;
	prev->next = s;
	next->prev = s;
}

static inline void s_list_insert(struct s_list * head, struct s_list * elem)
{
	_s_list_insert(elem, head, head->next);
}

static inline void s_list_insert_tail(struct s_list * head, struct s_list * elem)
{
	_s_list_insert(elem, head->prev, head);
}

static inline void s_list_del(struct s_list * s)
{
	s->prev->next = s->next;
	s->next->prev = s->prev;
	s_list_init(s);
}

#define s_list_first(head)	((head)->next)

#define s_list_empty(head)	((head)->next == (head))

#define s_list_foreach(p, head)	\
	for((p) = (head)->next; (p) != (head); (p) = (p)->next) 

#define s_list_foreach_safe(p, tmp, head)	\
	for((p) = (head)->next, (tmp) = (p)->next; (p) != (head); (p) = (tmp), (tmp) = (tmp)->next)

#define s_list_entry(p, t, e)	s_container_of(p, t, e)
#define s_list_first_entry(head, t, e) s_list_entry(s_list_first(head), t, e)

#endif


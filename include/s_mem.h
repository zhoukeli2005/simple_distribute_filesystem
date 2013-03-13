#ifndef s_mem_h_
#define s_mem_h_

#define s_malloc(t, n)	(t*)__s_malloc(sizeof(t) * (n))
#define s_free(p) __s_free(p)

void * __s_malloc(unsigned int sz);

void __s_free(void * p);

unsigned int 
s_mem_used( void );

#endif

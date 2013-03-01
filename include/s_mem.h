#ifndef s_mem_h_
#define s_mem_h_

#define s_malloc(t, n)	(t*)malloc(sizeof(t) * (n))
#define s_free(p) free(p)

#endif

#include <s_mem.h>
#include <s_common.h>

static unsigned int g_mem = 0;

void * __s_malloc(unsigned int sz)
{
	sz += sizeof(unsigned int);
	g_mem += sz;
	unsigned int * psz = malloc(sz);
	*psz = sz;
	return (void*)(psz + 1);
}

void __s_free(void * p)
{
	unsigned int * psz = (unsigned int*)p;
	psz--;
	g_mem -= (*psz);
	free(psz);
}

unsigned int s_mem_used( void )
{
	return g_mem;
}


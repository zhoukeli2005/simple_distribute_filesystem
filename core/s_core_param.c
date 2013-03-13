#include "s_core_param.h"
#include <s_mem.h>

#define S_CORE_PARAM_POOL_SIZE	128
#define S_CORE_PARAM_BUF_MAX_COUNT	512

// meta-data parameter
static struct s_core_metadata_param g_mdp_pool[S_CORE_PARAM_POOL_SIZE];
static S_LIST_DECLARE(g_mdp_free_list);
static int g_mdp_free_count = 0;

// file meta-data

void s_core_param_init( void )
{
	int i;

	// meta-data parameter
	for(i = 0; i < S_CORE_PARAM_POOL_SIZE; ++i) {
		struct s_core_metadata_param * mdp = &g_mdp_pool[i];
		s_list_insert(&g_mdp_free_list, &mdp->__link);
	}
	g_mdp_free_count = S_CORE_PARAM_POOL_SIZE;
}

/* -- metadata parameter -- */
struct s_core_metadata_param * s_core_metadata_param_create( void )
{
	if(!s_list_empty(&g_mdp_free_list)) {
		struct s_core_metadata_param * mdp = s_list_first_entry(&g_mdp_free_list, struct s_core_metadata_param, __link);
		s_list_del(&mdp->__link);
		g_mdp_free_count--;
		return mdp;
	}
	struct s_core_metadata_param * mdp = s_malloc(struct s_core_metadata_param, 1);
	if(!mdp) {
		s_log("no mem for s_core_metadata_param!");
		return NULL;
	}
	return mdp;
}

void s_core_metadata_param_destroy( struct s_core_metadata_param * mdp )
{
	int idx = mdp - &g_mdp_pool[0];
	if(idx < 0 && idx >= S_CORE_PARAM_POOL_SIZE && g_mdp_free_count > S_CORE_PARAM_BUF_MAX_COUNT) {
		// got by s_malloc and buf is full
		s_free(mdp);
		return;
	}
	g_mdp_free_count++;
	s_list_insert(&g_mdp_free_list, &mdp->__link);
}

/* -- file metadata -- */
struct s_file_meta_data * s_file_meta_data_create( int nblocks )
{
	int sz = sizeof(struct s_file_meta_data) + sizeof(unsigned short int) * nblocks;
	struct s_file_meta_data * md = (struct s_file_meta_data *)s_malloc(char, sz);
	if(!md) {
		s_log("[Error] no mem for file meta-data!");
		return NULL;
	}
	md->__nblocks = nblocks;
	md->nblocks = nblocks;
	return md;
}

void s_file_meta_data_destroy( struct s_file_meta_data * md )
{
	s_free(md);
}

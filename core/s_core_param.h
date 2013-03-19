#ifndef s_core_param_h_
#define s_core_param_h_

#include "s_core.h"

/* -- init core param -- */
void
s_core_param_init( void );

/* -- file meta data -- */
struct s_file_meta_data *
s_file_meta_data_create( struct s_core * core, int nblocks );

void
s_file_meta_data_destroy( struct s_file_meta_data * md );


/* -- file meta-meta data -- */
struct s_file_meta_meta_data *
s_file_meta_meta_data_create( struct s_core * core, int nmserv );

void
s_file_meta_meta_data_destroy( struct s_file_meta_meta_data * mmd );


/* -- metadata param -- */
struct s_core_metadata_param *
s_core_metadata_param_create( struct s_core * core );

void
s_core_metadata_param_destroy( struct s_core_metadata_param * param );


/* -- read-write param -- */
struct s_core_rw_param *
s_core_rw_param_create( struct s_core * core );

void
s_core_rw_param_destroy( struct s_core_rw_param * param );

#endif


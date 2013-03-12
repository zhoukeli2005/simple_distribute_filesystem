#ifndef s_core_h_
#define s_core_h_

#include <s_string.h>
#include <s_packet.h>
#include <s_net.h>
#include <s_server_group.h>
#include <s_common.h>

/* -- block size : 4M -- */
#define S_CORE_BLOCK_SIZE	4194304

/* -- max file size : 4T ( 1M blocks ) -- */
#define S_CORE_BLOCK_MAX_NUM	1048576

/* -- max errmsg length : 64 -- */
#define S_CORE_MAX_ERRMSG_LENGTH 64

/* -- Errno -- */
enum S_E_CORE_ERR {
	S_CORE_ERR_NOERR = 0,
	S_CORE_ERR_BAD_PARAM,
	
	_S_CORE_ERR_MAX_
};

struct s_file_desc {
	unsigned int file_id;
	unsigned int file_version;
};

struct s_block_desc {
//	struct s_file_desc fdesc;
	unsigned int block_id;
	unsigned int block_version;
};

struct s_file_size {
	unsigned int low;
	unsigned int high;
};

struct s_file_offset {
	unsigned int low;
	unsigned int high;
};

struct s_file_meta_data {
	struct s_string * filename;
	struct s_file_desc fdesc;
	struct s_file_size fsize;

	int nblocks;
	unsigned short int block_locate[1];
};

struct s_file_meta_meta_data {
	struct s_string * filename;
	struct s_file_desc fdesc;

	int nmserv;
	unsigned short int mserver_locate[1];
};

/*
 *	do net event
 *
 *
 */
void
s_core_do_net_event(struct s_server_group * servg, struct s_server * serv, struct s_packet * pkt);



/*
 *	api callback function
 *
 */
typedef void(* S_CORE_CALLBACK)(void * udata);


/*
 *	-----    APIs related with meta-data    -----
 *
 *
 *	all these api-functions require the `struct s_core_metadata_param *` parameter
 *
 *	when operation complete, `callback` will be called, with the `parameter` passed as the only parameter
 *
 */
struct s_core_metadata_param {
	/* -- callback -- */
	union {
		void * p;
		int i;
	} ud;

	S_CORE_CALLBACK callback;

	struct s_file_meta_data * result;

	int errno;
	char errmsg[S_CORE_MAX_ERRMSG_LENGTH];

	/* -- prameter -- */
	struct s_string * filename;
	struct s_file_size size;	// for create file/ expand fsize

	struct s_file_offset offset;
};

/*
 *	create file
 *
 *	parameters:
 *
 *		1. s_core_metadata_param.filename : file name
 *		2. s_core_metadata_param.size : file size
 *
 *	result:
 *
 *		1. s_core_metadata_param.result : file metadata 
 */
int
s_core_create_file(struct s_server_group * servg, struct s_core_metadata_param * param);

/*
 *	get file metadata
 *
 *	parameters:
 *
 *		1. s_core_metadata_param.filename : file name
 *		2. s_core_metadata_param.offset : from where
 *		3. s_core_metadata_param.size : size ( -1 means whole file )
 *
 *	result:
 *
 *		1. s_core_metadata_param.result : file metadata
 */
int
s_core_get_file_metadata(struct s_server_group * servg, struct s_core_metadata_param * param);

/*
 *	expand file size
 *
 *	parameters:
 *
 *		1. s_core_metadata_param.filename : file name
 *		2. s_core_metadata_param.size : file size 
 *
 *	result:
 *
 *		1. s_core_metadata_param.result : file metadata
 */
int
s_core_expand_fsize(struct s_server_group * servg, struct s_core_metadata_param * param);



/*
 *	-----    APIs related with read/write blocks    -----
 *
 *
 *	all thest api-functions require `struct s_core_rw_param *` parameter
 *
 *	when operation complete, `callback` will be called, with the `parameter` passed as the only parameter
 *
 */
struct s_core_rw_param {
	/* -- callback -- */
	union {
		void * p;
		int i;
	} ud;

	S_CORE_CALLBACK callback;

	int errno;
	char errmsg[S_CORE_MAX_ERRMSG_LENGTH];

	/* -- parameter -- */
	char * buf;
	int buf_len;

	struct s_file_desc fdesc;
	struct s_file_offset offset;
	struct s_file_size size;

	/* -- result or parameter -- */
	int nblocks;
	struct s_block_desc block_desc[1];
};

/*
 *	get block meta-data
 *
 *	parameters:
 *
 *		1. s_core_rw_param.fdesc : whick file
 *		2. s_core_rw_param.offset : from where
 *		3. s_core_rw_param.size : length
 *
 *	results:
 *		1. s_core_rw_param.nblocks : how many blocks related
 *		2. s_core_rw_param.block_desc : block descriptors related
 */
int
s_core_get_block_metadata(struct s_server_group * servg, struct s_core_rw_param * param );



/*
 *	write data
 *
 *	parameters:
 *
 *		1. s_core_rw_param.fdesc : whick file
 *		2. s_core_rw_param.offset : from where
 *		3. s_core_rw_param.buf : data to be written
 *		4. s_core_rw_param.buf_len : length of data to be written
 *		5. s_core_rw_param.nblocks : how many blocks related
 *		6. s_core_rw_param.block_desc : block descriptors related
 *
 *
 *	results:
 *		none
 */
int
s_core_write(struct s_server_group * servg, struct s_core_rw_param * param );


/*
 *	read data
 *
 *	parameters:
 *
 *		1. s_core_rw_param.fdesc : whick file
 *		2. s_core_rw_param.offset : from where
 *		3. s_core_rw_param.buf : buf for read
 *		4. s_core_rw_param.buf_len : length of data to be read
 *		
 *	results:
 *
 *		1. s_core_rw_param.nblocks : how many blocks related
 *		2. s_core_rw_param.block_desc : block descriptors related
 *		3. s_core_rw_param.buf : data has been read
 */
int
s_core_read(struct s_server_group * servg, struct s_core_rw_param * param );

#endif


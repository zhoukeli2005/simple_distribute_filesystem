#ifndef s_core_h_
#define s_core_h_

#include <s_string.h>
#include <s_list.h>
#include <s_packet.h>
#include <s_net.h>
#include <s_server_group.h>
#include <s_config.h>
#include <s_hash.h>
#include <s_mem.h>
#include <s_common.h>

/* -- block size : 4M -- */
#define S_CORE_BLOCK_BITS	22
#define S_CORE_BLOCK_SIZE	(1 << 22)

/* -- max file size : 4T ( 1M blocks ) -- */
#define S_CORE_MAX_FSZ_BITS	42
#define S_CORE_MAX_FSIZE	(1 << 42)
#define S_CORE_BLOCK_MAX_NUM	1048576

/* -- max errmsg length : 64 -- */
#define S_CORE_MAX_ERRMSG_LENGTH 64

/* -- Errno -- */
enum S_E_CORE_ERR {
	S_CORE_ERR_NOERR = 0,
	S_CORE_ERR_BAD_PARAM,
	S_CORE_ERR_INTERNAL,
	
	_S_CORE_ERR_MAX_
};

struct s_core;

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

struct s_block_locate {
	unsigned short int locate[2];
};

struct s_file_meta_data {
	/* -- internal -- */
	struct s_list __link;
	int __nblocks;

	struct s_core * core;


	/* -- public -- */
	struct s_string * fname;
	struct s_file_desc fdesc;
	struct s_file_size fsize;

	int nblocks;
	struct s_block_locate blocks[1];
};

#define S_FILE_MMD_FLAG_DELETED		1
#define S_FILE_MMD_FLAG_CREATING	2
#define S_FILE_MMD_FLAG_NORMAL		3

struct s_file_meta_meta_data {
	/* -- internal -- */
	struct s_list __link;
	int __nmserv;

	struct s_core * core;


	/* -- public -- */
	struct s_string * fname;
	struct s_file_desc fdesc;

	unsigned int flags;

	int nmserv;
	unsigned short int mservs[1];
};

struct s_mserver {
	struct s_hash * file_metadata;	// filename --> metadata
	struct s_hash * file_meta_metadata; // filename --> meta-meta-data
	struct s_hash * file_creating;	// filename --> struct s_core_mcreating
};

struct s_client {
	int __dummy;
};

struct s_dserv_file {
	struct s_file_meta_data * file_metadata;
};

struct s_dserver {
	struct s_hash * file_metadata;	// filename --> metadata
};

struct s_core_create_param {
	struct s_config * config;

	int type;
	int id;

	/* ud = all_established(core); */
	void * ud;
	void * (*all_established)(struct s_core * core);
	void(*update)(struct s_core * core, void * ud);
};

struct s_core {
	int type;
	int id;

	unsigned short int file_id;

	struct s_server_group * servg;

	union {
		struct s_client client;

		struct s_mserver mserv;

		struct s_dserver dserv;
	};

	struct s_core_create_param create_param;
};

#define s_core_client(c)	&((c)->client)
#define s_core_mserv(c)		&((c)->mserv)
#define s_core_dserv(c)		&((c)->dserv)

struct s_core *
s_core_create( struct s_core_create_param * param );


/*
 *	core poll
 *
 */
int
s_core_poll( struct s_core * core, int msec );


/*
 *	clint-api callback function
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
	/* -- internal -- */
	struct s_list __link;

	struct s_core * core;

	/* -- callback -- */
	union {
		void * p;
		int i;
	} ud;

	S_CORE_CALLBACK callback;

	struct s_file_meta_data * result;

	int error;
	char errmsg[S_CORE_MAX_ERRMSG_LENGTH];

	/* -- prameter -- */
	struct s_string * fname;
	struct s_file_size size;	// for create file/ expand fsize

	struct s_file_offset offset;
};

/*
 *	create file < client api >
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
s_core_create_file(struct s_core * core, struct s_core_metadata_param * param);

/*
 *	get file metadata < client api >
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
s_core_get_file_metadata(struct s_core * core, struct s_core_metadata_param * param);

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
s_core_expand_fsize(struct s_core * core, struct s_core_metadata_param * param);



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
	/* -- internal -- */
	struct s_list __link;
	int __nblocks;
	
	/* -- callback -- */
	union {
		void * p;
		int i;
	} ud;

	S_CORE_CALLBACK callback;

	int errer;
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
s_core_get_block_metadata(struct s_core * core, struct s_core_rw_param * param );



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
s_core_write(struct s_core * core, struct s_core_rw_param * param );


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
s_core_read(struct s_core * core, struct s_core_rw_param * param );

#endif


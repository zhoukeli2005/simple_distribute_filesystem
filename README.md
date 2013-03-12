1. data-structure
	<1> s_string	-- ref count				(ok)
	<2> s_array	-- ref count				(ok)
	<3> s_packet	-- ref count				(ok)
	<4> s_queue						(ok)
	<5> s_list						(ok)
	<6> s_hash(s_string/double/void * -> data block)	(4 hour)	(ok)
		debug record:
		<<<1>>> 在一个笔误上debug了近2个小时！
			s_hash_set(...)返回的是指向val的指针
			我一直当成了指向struct s_hash_node的指针
			这该怎么破！

								(total 4 hour)


2. schedule
	<1> server group (connect - auth - reconnect - keep alive - ping pong - roundup time)		(xxx hour)	(ok)
	<2> file descriptor (32-bit id + 32-bit version)						(0.5 hour)
	<3> block descriptor (file descriptor + 16-bit block id + 16-bit version)			(0.5 hour)
	<4> file meta-data										(4 hour)
		<<1>> filename
		<<2>> file descriptor
		<<3>> file size
		<<4>> locate blocks
	<4.x> file meta-data in meta-server( file meta-meta-data )					(2 hour)
		<<1>> filename
		<<2>> file descriptor
		<<3>> locate file meta-data

	<5> create_file_meta_data( udata, callback ) --> meta-data					(4 hour)
		<<1>> udata --> {
				filename,
				size,
				void * p
			}
	<6> get_file_meta_data( udata, callback ) --> meta-data						(4 hour)
		<<1>> udata --> {
				filename,
				block start,
				block end,
				void * p
			}
	<7> expand_file_size( udata, callback ) --> meta-data						(3 hour)
		<<1>> udata --> {
				filename,
				nsize,
				void * p
			}

	<8.x> get_block_metadata( udata ) --> block meta-data						(4 hour)

	<8> write_block( udata, callback ) --> none							(4 hour)
		<<1>> udata --> {
				buf,
				len,
				void * p
				nblock,
				block descriptor array[]
			}
	<9> read_block( udata, callback ) --> none							(4 hour)
		<<1>> udata --> {
				buf,
				len,
				void * p
				nblock,
				block descriptor array[]
			}
	<10> read_block_array( udata, callback ) --> none						(2 hour)
		<<1>> udata --> {
				buf,
				len,
				void * p
				nblock,
				block descriptor array[]
			}
	<11> write_block_array( udata, callback ) --> none						(2 hour)
		<<1>> udata --> {
				buf,
				len,
				void * p
				nblock,
				block descriptor array[]
			}
	<12> read_blocks( udata, callback ) --> none							(2 hour)
		<<1>> udata --> {
				buf,
				len,
				void * p
				nblock,
				block descriptor start
			}
	<13> write_blocks( udata, callback ) --> none							(2 hour)
		<<1>> udata --> {
				buf,
				len,
				void * p
				nblock,
				block descriptor start
			}

													(total 34 hour)

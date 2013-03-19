#ifndef s_core_create_h_
#define s_core_create_h_

/*
   1. client --(param:create)--> meta-serv

   2.      meta-serv (check auth) ...
	   meta-serv --(can create ?)--> all other meta-serv
	   meta-serv (set timeout -> failed)

   3.              all other meta-serv (check auth) ...

   4.      meta-serv <--(can or cannot)-- all other meta-serv

   5.      meta-serv --(decide auth and main/slave meta-serv)--> all other meta-serv

   6.              all meta-serv (create file-meta-meta-data)

   7.              main meta-serv (create file-meta-data) ...
		   main meta-serv --(file-meta-data)--> related data-serv
		   main meta-serv <--(accept)-- related data-serv

   8.      meta-serv <--(file-meta-data)-- main meta-serv

   9. client <--(file-meta-data)-- meta-serv

 */

#include <s_packet.h>

#include "s_core.h"

// mserv
struct s_core_mcreating {
	struct s_core * core;

	struct s_string * fname;
	struct s_server * client;
	struct s_file_size size;

	int ncheck;
};

void
s_core_mserv_create( struct s_server * serv, struct s_packet * pkt, void * ud );

void
s_core_mserv_create_check_auth( struct s_server * serv, struct s_packet * pkt, void * ud );

void
s_core_mserv_create_check_auth_back( struct s_server * serv, struct s_packet * pkt, void * ud );

void
s_core_mserv_create_cancel( struct s_server * serv, struct s_packet * pkt, void * ud );

void
s_core_mserv_create_decide( struct s_server * serv, struct s_packet * pkt, void * ud );

void
s_core_mserv_create_metadata( struct s_server * serv, struct s_packet * pkt, void * ud );

void
s_core_mserv_create_md_accept( struct s_server * serv, struct s_packet * pkt, void * ud );

// dserv
void
s_core_dserv_create_metadata( struct s_server * serv, struct s_packet * pkt, void * ud );

#endif


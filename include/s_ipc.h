#ifndef s_ipc_h_
#define s_ipc_h_

#include <s_packet.h>

enum S_ENUM_PKT_TYPE {
	S_PKT_TYPE_RPC_BACK_ = 1,

	S_PKT_TYPE_IDENTIFY,
	S_PKT_TYPE_IDENTIFY_BACK,
	S_PKT_TYPE_PING,
	S_PKT_TYPE_PONG,

	S_PKT_TYPE_USER_DEF_BEGIN_,

	// begin user-define message

	// create file
		S_PKT_TYPE_CREATE,				// client --> mserv
		S_PKT_TYPE_CREATE_CHECK_AUTH,			// mserv --> other mserv
		S_PKT_TYPE_CREATE_METADATA,			// main mserv --> data serv / mserv / slave mserv
		S_PKT_TYPE_CREATE_META_META_DATA,		// main mserv --> data serv / mserv / slave mserv
		S_PKT_TYPE_CREATE_METADATA_ACCEPT,		// dserv --> main mserv

	// expand file size


	// end
	S_PKT_TYPE_MAX_
};

#endif


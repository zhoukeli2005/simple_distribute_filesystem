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

	// expand file size
		// xxx....

	// write data
		S_PKT_TYPE_PUSH_DATA,
		S_PKT_TYPE_WRITE,

	// global lock
		S_PKT_TYPE_GLOBAL_LOCK,
		S_PKT_TYPE_GLOBAL_UNLOCK,

	// lock
		S_PKT_TYPE_LOCK_START,				// client --> dserv
		S_PKT_TYPE_LOCK_NEXT,				// dserv --> dserv
		S_PKT_TYPE_LOCK_UNLOCK,				// dserv --> dserv
		S_PKT_TYPE_LOCK_END,				// dserv --> client

	// Paxos
		S_PKT_TYPE_PAXOS_REQUEST,
		S_PKT_TYPE_PAXOS_RESPONSE,
		S_PKT_TYPE_PAXOS_ACCEPT,
		S_PKT_TYPE_PAXOS_LEARN,

	// end
	S_PKT_TYPE_MAX_
};

#endif


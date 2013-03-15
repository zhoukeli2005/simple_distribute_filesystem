#ifndef s_core_create_pkt_h_
#define s_core_create_pkt_h_

#include <s_packet.h>
#include <s_ipc.h>

#include "s_core.h"
#include "s_core_param.h"

struct s_packet *
s_core_create_pkt_create(struct s_core * core, struct s_core_metadata_param * mp);

#endif


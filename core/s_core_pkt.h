#ifndef s_core_pkt_h_
#define s_core_pkt_h_

#include <s_packet.h>
#include <s_ipc.h>

#include "s_core.h"
#include "s_core_param.h"

struct s_packet *
s_core_pkt_from_mdp(struct s_core * core, struct s_core_metadata_param * mp);

struct s_packet *
s_core_pkt_from_fmmd(struct s_core * core, struct s_file_meta_meta_data * mmd);

struct s_file_meta_meta_data *
s_core_pkt_to_fmmd(struct s_core * core, struct s_packet * pkt);

struct s_packet *
s_core_pkt_from_fmd(struct s_core * core, struct s_file_meta_data * md);

struct s_file_meta_data *
s_core_pkt_to_fmd(struct s_core * core, struct s_packet * pkt);

#endif


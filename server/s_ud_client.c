#include "s_server.h"

static void create_back(void * udata)
{
}

void * s_ud_client_init(struct s_core * core)
{
	struct s_core_metadata_param param;
	param.fname = s_string_create("test_file.xx");
	param.callback = &create_back;

	param.size.high = 0;
	param.size.low = 1026;

	s_core_create_file(core, &param);

	return NULL;
}

void s_ud_client_update(struct s_core * core, void * ud)
{
}

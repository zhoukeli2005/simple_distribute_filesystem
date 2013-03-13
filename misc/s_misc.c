#include <s_misc.h>
#include <s_common.h>

int s_util_get_serv_id(const char * p, char sep)
{
	char * p_ = strrchr(p, sep);
	if(!p_) {
		s_log("miss id!");
		return -1; 
	}   

	int id = atoi(p_ + 1); 
	return id;
}


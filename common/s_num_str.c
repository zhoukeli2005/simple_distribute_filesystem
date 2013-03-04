#include <s_num_str.h>
#include <s_common.h>

#define S_NUM_BUF_SZ	64

const char * s_int_to_string(int i)
{
	static char buf[S_NUM_BUF_SZ];
	buf[S_NUM_BUF_SZ - 1] = 0;
	int pos = S_NUM_BUF_SZ - 2;

	if(i == 0) {
		return "0";
	}

	int sign = 0;
	if(i < 0) {
		sign = 1;
		i *= -1;
	}

	while(i > 0 && pos >= 0) {
		int c = i % 10;
		i /= 10;
		buf[pos--] = c + '0';
	}

	if(i > 0) {
		s_log("num is to big!!");
		return NULL;
	}

	if(sign && pos >= 0) {
		buf[pos--] = '-';
	}

	return &buf[pos+1];
}

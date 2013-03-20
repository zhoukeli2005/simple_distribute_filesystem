#include "s_core_util.h"

#define S_SZ_OF_UINT	sizeof(unsigned int)
#define S_BITS_1_MASK(n)	((1 << (n)) - 1)
#define S_HIGH_BITS(w, n)	(((w) >> (S_SZ_OF_UINT - (n))) & S_BITS_1_MASK(n))

#define S_BITS_1_SHIFT_MASK(n, s)	(((1 << (n)) - 1) << s)

int s_core_fsize_to_block_num(struct s_file_size * size)
{
	unsigned long bits = S_SZ_OF_UINT - S_CORE_BLOCK_BITS;
	unsigned int num = S_HIGH_BITS(size->low, bits);
	if(S_SZ_OF_UINT < S_CORE_MAX_FSZ_BITS) {
		unsigned int high_bits = S_CORE_MAX_FSZ_BITS - S_SZ_OF_UINT;
		num |= size->high << bits & S_BITS_1_SHIFT_MASK(high_bits, bits);
	}

	if(size->low & S_BITS_1_MASK(S_CORE_BLOCK_BITS)) {
		num++;
	}
	return num;
}


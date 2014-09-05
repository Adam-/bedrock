#include "util/endian.h"

#include <stdint.h>

static uint16_t endian_test_i = 0x0001;
static char *endian_test_c = (char *) &endian_test_i;

#define IS_LITTLE_ENDIAN (*endian_test_c == 0x01)

void convert_endianness(unsigned char *data, size_t size)
{
	unsigned char *s = data, *e = data + size - 1;

	if (!IS_LITTLE_ENDIAN)
		return;

	for (; s < e; ++s, --e)
	{
		unsigned char t = *s;

		*s = *e;
		*e = t;
	}
}

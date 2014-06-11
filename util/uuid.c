#include "util/util.h"
#include "util/uuid.h"

void uuid_v3_generate(struct uuid *uuid)
{
	int i;

	for (i = 0; i < UUID_LEN; ++i)
		uuid->u[i] = rand();
	uuid->u[6] = 0x30 | (uuid->u[6] & 0xF);
	uuid->u[8] = 0xB0 | (uuid->u[8] & 0xF);
}

const char *uuid_to_string(struct uuid *uuid)
{
	static char buf[UUID_STR_LEN + 1];
	const char hex[] = "0123456789abcdef";
	int i;
	char *p = buf;

	for (i = 0; i < UUID_LEN; ++i)
	{
		*p++ = hex[uuid->u[i] >> 4];
		*p++ = hex[uuid->u[i] & 0xF];

		if (i == 3 || i == 5 || i == 7 || i == 9)
			*p++ = '-';
	}

	return buf;
}

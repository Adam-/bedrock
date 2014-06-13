#include "util/util.h"
#include "util/uuid.h"
#include <openssl/md5.h>

void uuid_v3_from_name(struct uuid *uuid, const char *name)
{
	MD5_CTX ctx;

	MD5_Init(&ctx);
	MD5_Update(&ctx, name, strlen(name));
	MD5_Final(uuid->u, &ctx);

	uuid->u[6] = 0x30 | (uuid->u[6] & 0xF);
	uuid->u[8] = 0x80 | (uuid->u[8] & 0x3F);
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

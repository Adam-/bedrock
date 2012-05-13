#include "server/client.h"

enum
{
	KEEP_ALIVE = 0x00,
	LOGIN_REQUEST = 0x01
};

extern int packet_parse(bedrock_client *client, const unsigned char *buffer, size_t len);

extern int packet_read_int(const unsigned char *buffer, size_t offset, void *dest, size_t dest_size);

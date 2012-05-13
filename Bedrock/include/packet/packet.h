#include "server/client.h"

enum
{
	KEEP_ALIVE    = 0x00,
	LOGIN_REQUEST = 0x01,
	HANDSHAKE     = 0x02
};

enum
{
	ALLOW_UNAUTHED = 1 << 0
};

extern int packet_parse(bedrock_client *client, const unsigned char *buffer, size_t len);

extern void packet_read_int(const unsigned char *buffer, size_t buffer_size, size_t *offset, void *dest, size_t dest_size);
extern void packet_read_string(const unsigned char *buffer, size_t buffer_size, size_t *offset, char *dest, size_t dest_size);

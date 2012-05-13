#include "server/client.h"

enum
{
	KEEP_ALIVE = 0x00,
	LOGIN_REQUEST = 0x01
};

extern int parse_incoming_packet(bedrock_client *client, const unsigned char *buffer, size_t len);

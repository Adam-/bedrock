#include "server/client.h"

enum
{
	KEEP_ALIVE            = 0x00,
	LOGIN_REQUEST         = 0x01,
	HANDSHAKE             = 0x02,
	PLAYER                = 0x0A,
	PLAYER_POS            = 0x0B,
	PLAYER_POS_LOOK       = 0x0D,
	MAP_COLUMN_ALLOCATION = 0x32,
	MAP_COLUMN            = 0x33
};

enum
{
	HARD_SIZE = 1 << 0
};

enum
{
	ERROR_UNKNOWN        = 0,
	ERROR_EAGAIN         = -1,
	ERROR_INVALID_FORMAT = -2,
	ERROR_UNEXPECTED     = -3
};

extern int packet_parse(struct bedrock_client *client, const unsigned char *buffer, size_t len);

extern void packet_read_int(const unsigned char *buffer, size_t buffer_size, size_t *offset, void *dest, size_t dest_size);
extern void packet_read_string(const unsigned char *buffer, size_t buffer_size, size_t *offset, char *dest, size_t dest_size);

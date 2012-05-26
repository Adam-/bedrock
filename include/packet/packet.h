#include "server/client.h"

enum
{
	KEEP_ALIVE                    = 0x00,
	LOGIN_REQUEST                 = 0x01,
	HANDSHAKE                     = 0x02,
	CHAT_MESSAGE                  = 0x03,
	SPAWN_POINT                   = 0x06,
	PLAYER                        = 0x0A,
	PLAYER_POS                    = 0x0B,
	PLAYER_LOOK                   = 0x0C,
	PLAYER_POS_LOOK               = 0x0D,
	HELD_ITEM_CHANGE              = 0x10,
	ENTITY_ACTION                 = 0x13,
	SPAWN_NAMED_ENTITY            = 0x14,
	DESTROY_ENTITY                = 0x1D,
	ENTITY_RELATIVE_MOVE          = 0x1F,
	ENTITY_LOOK_AND_RELATIVE_MOVE = 0x21,
	ENTITY_TELEPORT               = 0x22,
	ENTITY_HEAD_LOOK              = 0x23,
	MAP_COLUMN_ALLOCATION         = 0x32,
	MAP_COLUMN                    = 0x33,
	CLOSE_WINDOW                  = 0x65,
	SET_SLOT                      = 0x67,
	PLAYER_LIST                   = 0xC9,
	DISCONNECT                    = 0xFF
};

enum
{
	SOFT_SIZE = 1 << 0,
	HARD_SIZE = 1 << 1
};

enum
{
	ERROR_UNKNOWN        = 0,
	ERROR_EAGAIN         = -1,
	ERROR_INVALID_FORMAT = -2,
	ERROR_UNEXPECTED     = -3
};

#define PACKET_HEADER_LENGTH 1

extern int packet_parse(struct bedrock_client *client, const unsigned char *buffer, size_t len);

extern void packet_read_int(const unsigned char *buffer, size_t buffer_size, size_t *offset, void *dest, size_t dest_size);
extern void packet_read_string(const unsigned char *buffer, size_t buffer_size, size_t *offset, char *dest, size_t dest_size);

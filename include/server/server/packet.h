#ifndef BEDROCK_SERVER_PACKET_H
#define BEDROCK_SERVER_PACKET_H

#include "util/buffer.h"

typedef bedrock_buffer bedrock_packet;

#include "server/client.h"
#include "blocks/items.h"

enum
{
	KEEP_ALIVE                    = 0x00,
	LOGIN_RESPONSE                = 0x01,
	HANDSHAKE                     = 0x02,
	CHAT_MESSAGE                  = 0x03,
	TIME                          = 0x04,
	ENTITY_EQUIPMENT              = 0x05,
	SPAWN_POINT                   = 0x06,
	PLAYER                        = 0x0A,
	PLAYER_POS                    = 0x0B,
	PLAYER_LOOK                   = 0x0C,
	PLAYER_POS_LOOK               = 0x0D,
	PLAYER_DIGGING                = 0x0E,
	PLAYER_BLOCK_PLACEMENT        = 0x0F,
	HELD_ITEM_CHANGE              = 0x10,
	ENTITY_ANIMATION              = 0x12,
	ENTITY_ACTION                 = 0x13,
	SPAWN_NAMED_ENTITY            = 0x14,
	SPAWN_DROPPED_ITEM            = 0x15,
	COLLECT_ITEM                  = 0x16,
	DESTROY_ENTITY                = 0x1D,
	ENTITY_TELEPORT               = 0x22,
	ENTITY_HEAD_LOOK              = 0x23,
	ENTITY_METADATA               = 0x28,
	MAP_COLUMN                    = 0x33,
	BLOCK_CHANGE                  = 0x35,
	MAP_COLUMN_BULK               = 0x38,
	CLOSE_WINDOW                  = 0x65,
	CLICK_WINDOW                  = 0x66,
	SET_SLOT                      = 0x67,
	CONFIRM_TRANSACTION           = 0x6A,
	PLAYER_LIST                   = 0xC9,
	CLIENT_SETTINGS               = 0xCC,
	CLIENT_STATUS                 = 0xCD,
	ENCRYPTION_RESPONSE           = 0xFC,
	ENCRYPTION_REQUEST            = 0xFD,
	LIST_PING                     = 0xFE,
	DISCONNECT                    = 0xFF
};

enum
{
	NONE        = 0,
	/* Size of this packet is not definite. Will be at least 'len' nutes */
	SOFT_SIZE   = 1 << 0,
	/* Only servers may send this packet */
	SERVER_ONLY = 1 << 1,
	/* Only clients may send this packet */
	CLIENT_ONLY = 1 << 2
};

enum
{
	ERROR_UNKNOWN        = 0,
	ERROR_EAGAIN         = -1,
	ERROR_INVALID_FORMAT = -2,
	ERROR_UNEXPECTED     = -3,
	ERROR_NOT_ALLOWED    = -4
};

struct client;

struct packet_info
{
	uint8_t id;
	uint8_t len;
	uint8_t permission;
	uint8_t flags;
	int (*handler)(struct client *, const bedrock_packet *);
};

#define PACKET_HEADER_LENGTH 1

extern struct packet_info packet_handlers[];

extern struct packet_info *packet_find(uint8_t id);
extern void packet_init(bedrock_packet *packet, uint8_t id);
extern void packet_free(bedrock_packet *packet);

extern int packet_parse(struct client *client, const bedrock_packet *packet);

extern void packet_read(const bedrock_packet *packet, size_t *offset, void *dest, size_t dest_size);
extern void packet_read_int(const bedrock_packet *packet, size_t *offset, void *dest, size_t dest_size);
extern void packet_read_string(const bedrock_packet *packet, size_t *offset, char *dest, size_t dest_size);
extern void packet_read_slot(const bedrock_packet *packet, size_t *offset, struct item_stack *stack);

extern void packet_pack_header(bedrock_packet *packet, uint8_t header);
extern void packet_pack(bedrock_packet *packet, const void *data, size_t size);
extern void packet_pack_int(bedrock_packet *packet, const void *data, size_t size);
extern void packet_pack_string(bedrock_packet *packet, const char *string);
extern void packet_pack_string_len(bedrock_packet *packet, const char *string, uint16_t len);
extern void packet_pack_slot(bedrock_packet *packet, struct item_stack *stack);

#endif // BEDROCK_SERVER_PACKET_H

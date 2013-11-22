#ifndef BEDROCK_SERVER_PACKET_H
#define BEDROCK_SERVER_PACKET_H

#include "util/buffer.h"

enum packet_error
{
	ERROR_OK             =  0,
	ERROR_UNKNOWN        = -1,
	ERROR_EAGAIN         = -2,
	ERROR_INVALID_FORMAT = -3,
	ERROR_UNEXPECTED     = -4,
	ERROR_NOT_ALLOWED    = -5
};


struct bedrock_packet
{
	bedrock_buffer buffer;
	size_t offset; // Current read offset
	enum packet_error error; // Error reading?
};
typedef struct bedrock_packet bedrock_packet;

#include "server/client.h"
#include "blocks/items.h"

/* Packets from non registered clients */
enum unknown_packet_id
{
	UNKNOWN_HANDSHAKE = 0x00
};

/* Packets from clients in status state */
enum status_packet_client_id
{
	STATUS_CLIENT_REQUEST = 0x00,
	STATUS_CLIENT_PING    = 0x01
};

/* Packets from the server to clients in the status state */
enum status_packet_server_id
{
	STATUS_SERVER_RESPONSE = 0x00,
	STATUS_SERVER_PING     = 0x01
};

/* Packets from clients in logging in state */
enum login_packet_client_id
{
	LOGIN_CLIENT_START               = 0x00,
	LOGIN_CLIENT_ENCRYPTION_RESPONSE = 0x01
};

/* Packets from the server to clients in logging in state */
enum login_packet_server_id
{
	LOGIN_SERVER_DISCONNECT         = 0x00,
	LOGIN_SERVER_ENCRYPTION_REQUEST = 0x01,
	LOGIN_SERVER_LOGIN_SUCCESS      = 0x02
};

/* Packets from fully registered clients */
enum client_packet_id
{
	CLIENT_KEEP_ALIVE                    = 0x00,
	CLIENT_CHAT_MESSAGE                  = 0x01,
	CLIENT_PLAYER                        = 0x03,
	CLIENT_PLAYER_POS                    = 0x04,
	CLIENT_PLAYER_LOOK                   = 0x05,
	CLIENT_PLAYER_POS_LOOK               = 0x06,
	CLIENT_PLAYER_DIGGING                = 0x07,
	CLIENT_PLAYER_BLOCK_PLACEMENT        = 0x08,
	CLIENT_HELD_ITEM_CHANGE              = 0x09,
	CLIENT_ANIMATION                     = 0x0A,
	CLIENT_ENTITY_ACTION                 = 0x0B,
	CLIENT_CLOSE_WINDOW                  = 0x0D,
	CLIENT_CLICK_WINDOW                  = 0x0E,
	CLIENT_CONFIRM_TRANSACTION           = 0x0F,
	CLIENT_CREATIVE_INVENTORY_ACTION     = 0x10,
	CLIENT_PLAYER_ABILITIES              = 0x13,
	CLIENT_CLIENT_SETTINGS               = 0x15,
	CLIENT_CLIENT_STATUS                 = 0x16,
	CLIENT_PLUGIN_MESSAGE                = 0x17,
};

/* Packets from the server */
enum server_packet_id
{
	SERVER_KEEP_ALIVE                    = 0x00,
	SERVER_JOIN_GAME                     = 0x01,
	SERVER_CHAT_MESSAGE                  = 0x02,
	SERVER_TIME                          = 0x03,
	SERVER_ENTITY_EQUIPMENT              = 0x04,
	SERVER_SPAWN_POINT                   = 0x05,
	SERVER_PLAYER_POS_LOOK               = 0x08,
	SERVER_HELD_ITEM_CHANGE              = 0x09,
	SERVER_ANIMATION                     = 0x0B,
	SERVER_SPAWN_PLAYER                  = 0x0C,
	SERVER_COLLECT_ITEM                  = 0x0D,
	SERVER_SPAWN_OBJECT                  = 0x0E,
	SERVER_DESTROY_ENTITY                = 0x13,
	SERVER_ENTITY_TELEPORT               = 0x18,
	SERVER_ENTITY_HEAD_LOOK              = 0x19,
	SERVER_ENTITY_METADATA               = 0x1C,
	SERVER_ENTITY_PROPERTIES             = 0x20,
	SERVER_MAP_COLUMN                    = 0x21,
	SERVER_BLOCK_CHANGE                  = 0x23,
	SERVER_MAP_COLUMN_BULK               = 0x26,
	SERVER_CHANGE_GAME_STATE             = 0x2B,
	SERVER_OPEN_WINDOW                   = 0x2D,
	SERVER_CLOSE_WINDOW                  = 0x2E,
	SERVER_SET_SLOT                      = 0x2F,
	SERVER_UPDATE_WINDOW_PROPERTY        = 0x30,
	SERVER_CONFIRM_TRANSACTION           = 0x32,
	SERVER_PLAYER_LIST                   = 0x38,
	SERVER_PLAYER_ABILITIES              = 0x39,
	SERVER_PLUGIN_MESSAGE                = 0x3F,
	SERVER_DISCONNECT                    = 0x40
};

struct client;

/* Packet info for packets received from the client */
struct client_packet_info
{
	enum client_packet_id id;
	int (*handler)(struct client *, bedrock_packet *);
};

extern struct client_packet_info client_unknown_packet_handlers[], client_status_packet_handlers[], client_login_packet_handlers[], client_packet_handlers[];

extern void packet_init(bedrock_packet *packet, packet_id id);
extern void packet_free(bedrock_packet *packet);

extern int packet_parse(struct client *client, bedrock_packet *packet);

extern void packet_read(bedrock_packet *packet, void *dest, size_t dest_size);
extern void packet_read_int(bedrock_packet *packet, void *dest, size_t dest_size);
extern void packet_read_varint(bedrock_packet *packet, int32_t *dest);
extern void packet_read_varuint(bedrock_packet *packet, uint32_t *dest);
extern void packet_read_string(bedrock_packet *packet, char *dest, size_t dest_size);
extern void packet_read_slot(bedrock_packet *packet, struct item_stack *stack);

extern void packet_pack(bedrock_packet *packet, const void *data, size_t size);
extern void packet_pack_int(bedrock_packet *packet, const void *data, size_t size);
extern void packet_pack_varint(bedrock_packet *packet, int32_t data);
extern void packet_pack_varuint(bedrock_packet *packet, uint32_t data);
extern void packet_pack_string(bedrock_packet *packet, const char *string);
extern void packet_pack_string_len(bedrock_packet *packet, const char *string, uint16_t len);
extern void packet_pack_slot(bedrock_packet *packet, struct item_stack *stack);

#endif // BEDROCK_SERVER_PACKET_H

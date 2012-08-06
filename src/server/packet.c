#include "server/bedrock.h"
#include "server/packet.h"
#include "util/endian.h"

#include "packet/packet_keep_alive.h"
#include "packet/packet_handshake.h"
#include "packet/packet_chat_message.h"
#include "packet/packet_player.h"
#include "packet/packet_position.h"
#include "packet/packet_player_look.h"
#include "packet/packet_position_and_look.h"
#include "packet/packet_player_digging.h"
#include "packet/packet_block_placement.h"
#include "packet/packet_held_item_change.h"
#include "packet/packet_entity_animation.h"
#include "packet/packet_entity_action.h"
#include "packet/packet_close_window.h"
#include "packet/packet_click_window.h"
#include "packet/packet_client_status.h"
#include "packet/packet_encryption_response.h"
#include "packet/packet_list_ping.h"
#include "packet/packet_disconnect.h"

struct packet_info packet_handlers[] = {
	{KEEP_ALIVE,              5, STATE_BURSTING | STATE_IN_GAME,   HARD_SIZE,               packet_keep_alive},
	{LOGIN_RESPONSE,         12, 0,                                      SOFT_SIZE | SERVER_ONLY, NULL},
	{HANDSHAKE,              10, STATE_UNAUTHENTICATED,                  SOFT_SIZE,               packet_handshake},
	{CHAT_MESSAGE,            3, STATE_IN_GAME,                          SOFT_SIZE,               packet_chat_message},
	{TIME,                    9, 0,                                      HARD_SIZE | SERVER_ONLY, NULL},
	{ENTITY_EQUIPMENT,       11, 0,                                      HARD_SIZE | SERVER_ONLY, NULL},
	{SPAWN_POINT,            13, 0,                                      HARD_SIZE | SERVER_ONLY, NULL},
	{PLAYER,                  2, STATE_BURSTING | STATE_IN_GAME,         HARD_SIZE,               packet_player},
	{PLAYER_POS,             34, STATE_BURSTING | STATE_IN_GAME,         HARD_SIZE,               packet_position},
	{PLAYER_LOOK,            10, STATE_IN_GAME,                          HARD_SIZE,               packet_player_look},
	{PLAYER_POS_LOOK,        42, STATE_BURSTING | STATE_IN_GAME,         HARD_SIZE,               packet_position_and_look},
	{PLAYER_DIGGING,         12, STATE_IN_GAME,                          HARD_SIZE,               packet_player_digging},
	{PLAYER_BLOCK_PLACEMENT, 13, STATE_IN_GAME,                          SOFT_SIZE,               packet_block_placement},
	{HELD_ITEM_CHANGE,        3, STATE_IN_GAME,                          HARD_SIZE,               packet_held_item_change},
	{ENTITY_ANIMATION,        6, STATE_IN_GAME,                          HARD_SIZE,               packet_entity_animation},
	{ENTITY_ACTION,           6, STATE_IN_GAME,                          HARD_SIZE,               packet_entity_action},
	{SPAWN_NAMED_ENTITY,     23, 0,                                      SOFT_SIZE | SERVER_ONLY, NULL},
	{SPAWN_DROPPED_ITEM,     25, 0,                                      HARD_SIZE | SERVER_ONLY, NULL},
	{COLLECT_ITEM,            9, 0,                                      HARD_SIZE | SERVER_ONLY, NULL},
	{DESTROY_ENTITY,          6, 0,                                      SOFT_SIZE | SERVER_ONLY, NULL},
	{ENTITY_TELEPORT,        19, 0,                                      HARD_SIZE | SERVER_ONLY, NULL},
	{ENTITY_HEAD_LOOK,        6, 0,                                      HARD_SIZE | SERVER_ONLY, NULL},
	{ENTITY_METADATA,         5, 0,                                      SOFT_SIZE | SERVER_ONLY, NULL},
	{MAP_COLUMN,             17, 0,                                      SOFT_SIZE | SERVER_ONLY, NULL},
	{BLOCK_CHANGE,           13, 0,                                      HARD_SIZE | SERVER_ONLY, NULL},
	{MAP_COLUMN_BULK,         7, 0,                                      SOFT_SIZE | SERVER_ONLY, NULL},
	{CLOSE_WINDOW,            2, STATE_IN_GAME,                          HARD_SIZE,               packet_close_window},
	{CLICK_WINDOW,            8, STATE_IN_GAME,                          SOFT_SIZE,               packet_click_window},
	{SET_SLOT,                4, 0,                                      SOFT_SIZE | SERVER_ONLY, NULL},
	{CONFIRM_TRANSACTION,     5, 0,                                      HARD_SIZE | SERVER_ONLY, NULL},
	{PLAYER_LIST,             6, 0,                                      SOFT_SIZE | SERVER_ONLY, NULL},
	{CLIENT_STATUS,           2, STATE_LOGGED_IN | STATE_IN_GAME,        HARD_SIZE,               packet_client_status},
	{ENCRYPTION_RESPONSE,     5, STATE_HANDSHAKING,                      SOFT_SIZE,               packet_encryption_response},
	{ENCRYPTION_REQUEST,      7, 0,                                      SOFT_SIZE | SERVER_ONLY, NULL},
	{LIST_PING,               1, STATE_UNAUTHENTICATED,                  HARD_SIZE,               packet_list_ping},
	{DISCONNECT,              3, STATE_ANY,                              SOFT_SIZE,               packet_disconnect}
};

static int packet_compare(const uint8_t *id, const struct packet_info *handler)
{
	if (*id < handler->id)
		return -1;
	else if (*id > handler->id)
		return 1;
	return 0;
}

typedef int (*compare_func)(const void *, const void *);

struct packet_info *packet_find(uint8_t id)
{
	return bsearch(&id, packet_handlers, sizeof(packet_handlers) / sizeof(struct packet_info), sizeof(struct packet_info), (compare_func) packet_compare);
}

void packet_init(bedrock_packet *packet, uint8_t id)
{
	struct packet_info *handler = packet_find(id);
	
	bedrock_assert(packet != NULL && handler != NULL, return);

	snprintf(packet->name, sizeof(packet->name), "client packet 0x%02x", id);
	packet->data = bedrock_malloc(handler->len);
	packet->length = 0;
	packet->capacity = handler->len;
}

void packet_free(bedrock_packet *packet)
{
	bedrock_free(packet->data);
}

/** Parse a packet. Returns -1 if the packet is invalid or unexpected, 0 if there is not
 * enough data yet, or the amount of data read from buffer.
 */
int packet_parse(struct bedrock_client *client, const bedrock_packet *packet)
{
	uint8_t id = *packet->data;
	int i;

	struct packet_info *handler = packet_find(id);
	if (handler == NULL)
	{
		bedrock_log(LEVEL_WARN, "packet: Unrecognized packet 0x%02x from %s", id, client_get_ip(client));
		client->in_buffer_len = 0;
		return -1;
	}

	if (handler->flags & SERVER_ONLY)
	{
		bedrock_log(LEVEL_WARN, "packet: Unexpected server only packet 0x%02x from client %s - dropping client", id, client_get_ip(client));
		client_exit(client);
		return -1;
	}

	if (packet->length < handler->len)
		return 0;

	if ((handler->permission & client->authenticated) == 0)
	{
		bedrock_log(LEVEL_WARN, "packet: Unexpected packet 0x%02x from client %s - dropping client", id, client_get_ip(client));
		client_exit(client);
		return -1;
	}

	bedrock_log(LEVEL_PACKET_DEBUG, "packet: Got packet 0x%02x from %s (%s)", id, *client->name ? client->name : "(unknown)", client_get_ip(client));

	i = handler->handler(client, packet);

	if (i <= 0)
	{
		const char *error;

		switch (i)
		{
			default:
			case ERROR_UNKNOWN:
				error = "unknown error";
				break;
			case ERROR_EAGAIN:
				return 0;
			case ERROR_INVALID_FORMAT:
				error = "invalid format";
				break;
			case ERROR_UNEXPECTED:
				error = "unexpected";
				break;
			case ERROR_NOT_ALLOWED:
				error = "not allowed";
				break;
		}

		bedrock_log(LEVEL_WARN, "packet: Invalid packet 0x%02x from %s - %s, dropping client", id, client_get_ip(client), error);
		client_exit(client);
		return -1;
	}

	if (handler->flags & HARD_SIZE)
		if (i != handler->len)
		{
			bedrock_log(LEVEL_WARN, "packet: Packet 0x%02x from client %s was handled improperly, expected %d and got %d - dropping client", id, client_get_ip(client), handler->len, i);
			client_exit(client);
			return -1;
		}

	return i;
}

void packet_read(const bedrock_packet *packet, size_t *offset, void *dest, size_t dest_size)
{
	if (*offset <= ERROR_UNKNOWN)
		return;
	else if (*offset + dest_size > packet->length)
	{
		*offset = ERROR_EAGAIN;
		return;
	}

	memcpy(dest, packet->data + *offset, dest_size);
	*offset += dest_size;
}

void packet_read_int(const bedrock_packet *packet, size_t *offset, void *dest, size_t dest_size)
{
	if (*offset <= ERROR_UNKNOWN)
		return;
	else if (*offset + dest_size > packet->length)
	{
		*offset = ERROR_EAGAIN;
		return;
	}

	memcpy(dest, packet->data + *offset, dest_size);
	convert_endianness(dest, dest_size);
	*offset += dest_size;
}

void packet_read_string(const bedrock_packet *packet, size_t *offset, char *dest, size_t dest_size)
{
	uint16_t length, i, j;

	bedrock_assert(dest != NULL && dest_size > 0, goto error);

	packet_read_int(packet, offset, &length, sizeof(length));

	*dest = 0;

	if (*offset <= ERROR_UNKNOWN)
		return;
	/* Remember, this length is length in CHARACTERS */
	else if (*offset + (length * 2) > packet->length)
	{
		*offset = ERROR_EAGAIN;
		return;
	}

	if (length >= dest_size)
	{
		*offset = ERROR_EAGAIN;
		return;
	}
	else if (length > BEDROCK_MAX_STRING_LENGTH - 1)
	{
		*offset = ERROR_INVALID_FORMAT;
		return;
	}

	bedrock_assert(length < dest_size, goto error);

	for (i = 0, j = 1; i < length; ++i, j += 2)
		dest[i] = *(packet->data + *offset + j);
	dest[length] = 0;

	*offset += length * 2;
	return;

 error:
	*offset = ERROR_UNKNOWN;
}

void packet_pack_header(bedrock_packet *packet, uint8_t header)
{
	packet_pack_int(packet, &header, sizeof(header));
}

void packet_pack(bedrock_packet *packet, const void *data, size_t size)
{
	bedrock_assert(packet != NULL && data != NULL, return);

	bedrock_buffer_append(packet, data, size);
}

void packet_pack_int(bedrock_packet *packet, const void *data, size_t size)
{
	size_t old_len;

	bedrock_assert(packet != NULL && data != NULL, return);

	old_len = packet->length;
	packet_pack(packet, data, size);
	if (old_len + size == packet->length)
		convert_endianness(packet->data + old_len, size);
}

void packet_pack_string(bedrock_packet *packet, const char *string)
{
	uint16_t len, i;

	bedrock_assert(packet != NULL && string != NULL, return);

	len = strlen(string);

	packet_pack_int(packet, &len, sizeof(len));

	bedrock_buffer_ensure_capacity(packet, len * 2);

	for (i = 0; i < len; ++i)
	{
		packet->data[packet->length++] = 0;
		packet->data[packet->length++] = *string++;
	}
}


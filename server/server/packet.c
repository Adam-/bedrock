#include "server/bedrock.h"
#include "server/packet.h"
#include "server/packets.h"
#include "util/endian.h"

#define DEFAULT_PACKET_SIZE 16

struct client_packet_info client_unknown_packet_handlers[] = {
	{UNKNOWN_HANDSHAKE, packet_handshake}
};

struct client_packet_info client_status_packet_handlers[] = {
	{STATUS_CLIENT_REQUEST, packet_status_request},
	{STATUS_CLIENT_PING,    packet_ping}
};

struct client_packet_info client_login_packet_handlers[] = {
	{LOGIN_CLIENT_START,               packet_login},
	{LOGIN_CLIENT_ENCRYPTION_RESPONSE, packet_encryption_response}
};

struct client_packet_info client_packet_handlers[] = {
	{CLIENT_KEEP_ALIVE,                packet_keep_alive},
	{CLIENT_CHAT_MESSAGE,              packet_chat_message},
	{CLIENT_PLAYER,                    packet_player},
	{CLIENT_PLAYER_POS,                packet_position},
	{CLIENT_PLAYER_LOOK,               packet_player_look},
	{CLIENT_PLAYER_POS_LOOK,           packet_position_and_look},
	{CLIENT_PLAYER_DIGGING,            packet_player_digging},
	{CLIENT_PLAYER_BLOCK_PLACEMENT,    packet_block_placement},
	{CLIENT_HELD_ITEM_CHANGE,          packet_held_item_change},
	{CLIENT_ANIMATION,                 packet_entity_animation},
	{CLIENT_ENTITY_ACTION,             packet_entity_action},
	{CLIENT_CLOSE_WINDOW,              packet_close_window},
	{CLIENT_CLICK_WINDOW,              packet_click_window},
	{CLIENT_CONFIRM_TRANSACTION,       packet_confirm_transaction},
	{CLIENT_CREATIVE_INVENTORY_ACTION, packet_creative_inventory_action},
	{CLIENT_PLAYER_ABILITIES,          packet_player_abilities},
	{CLIENT_CLIENT_SETTINGS,           packet_client_settings},
	{CLIENT_CLIENT_STATUS,             packet_client_status},
	{CLIENT_PLUGIN_MESSAGE,            packet_plugin_message},
};

static int client_packet_compare(const packet_id *id, const struct client_packet_info *handler)
{
	if (*id < handler->id)
		return -1;
	else if (*id > handler->id)
		return 1;
	return 0;
}

typedef int (*compare_func)(const void *, const void *);

void packet_init(bedrock_packet *packet, packet_id id)
{
	snprintf(packet->buffer.name, sizeof(packet->buffer.name), "client packet 0x%02x", id);
	packet->buffer.data = bedrock_malloc(DEFAULT_PACKET_SIZE);
	packet->buffer.length = 0;
	packet->buffer.capacity = DEFAULT_PACKET_SIZE;
	packet->offset = 0;
	packet->error = ERROR_OK;

	packet_pack_varuint(packet, id);
}

void packet_free(bedrock_packet *packet)
{
	bedrock_free(packet->buffer.data);
}

/** Parse a packet. Returns <0 if the packet is invalid or unexpected, 0 if there is not
 * enough data yet, or the amount of data read from buffer.
 */
int packet_parse(struct client *client, bedrock_packet *packet)
{
	int i;
	struct client_packet_info *handler;
	packet_id id;
	uint32_t length;
	size_t s;
	struct client_packet_info *handlers;
	size_t size;

	packet_read_varuint(packet, &length);
	if (packet->error)
		return packet->error;

	// Length is the length of the remaining packet, including the following id
	s = packet->offset;
	// Total length expected to be s + length

	packet_read_varuint(packet, &id);

	if (packet->error)
		return packet->error;

	bedrock_log(LEVEL_PACKET_DEBUG, "packet: Got packet 0x%02x from %s (%s) state: %d", id, *client->name ? client->name : "(unknown)", client_get_ip(client), client->state);

	switch (client->state)
	{
		case STATE_UNAUTHENTICATED:
			handlers = client_unknown_packet_handlers;
			size = sizeof(client_unknown_packet_handlers);
			break;
		case STATE_STATUS:
			handlers = client_status_packet_handlers;
			size = sizeof(client_status_packet_handlers);
			break;
		case STATE_LOGIN:
			handlers = client_login_packet_handlers;
			size = sizeof(client_login_packet_handlers);
			break;
		default:
			handlers = client_packet_handlers;
			size = sizeof(client_packet_handlers);
	}

	handler = bsearch(&id, handlers, size / sizeof(struct client_packet_info), sizeof(struct client_packet_info), (compare_func) client_packet_compare);
	if (handler == NULL)
	{
		bedrock_log(LEVEL_WARN, "packet: Unrecognized packet 0x%02x from %s", id, client_get_ip(client));
		client->in_buffer_len = 0;
		return -1;
	}

	if (packet->buffer.length < s + length)
		return 0;

	i = handler->handler(client, packet);

	if (i != ERROR_OK || packet->error)
	{
		const char *error;

		switch (i != ERROR_OK ? i : packet->error)
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

	if (s + length != packet->offset)
	{
		bedrock_log(LEVEL_WARN, "packet: Length mismatch for packet 0x%02x from client %s, was told %lu but parsed %lu - dropping client", id, client_get_ip(client), s + length, packet->offset);
		client_exit(client);
		return -1;
	}

	return s + length;
}

void packet_read(bedrock_packet *packet, void *dest, size_t dest_size)
{
	if (packet->error)
		return;
	if (packet->offset + dest_size > packet->buffer.length)
	{
		packet->error = ERROR_EAGAIN;
		return;
	}

	memcpy(dest, packet->buffer.data + packet->offset, dest_size);
	packet->offset += dest_size;
}

void packet_read_int(bedrock_packet *packet, void *dest, size_t dest_size)
{
	if (packet->error)
		return;
	else if (packet->offset + dest_size > packet->buffer.length)
	{
		packet->error = ERROR_EAGAIN;
		return;
	}

	memcpy(dest, packet->buffer.data + packet->offset, dest_size);
	convert_endianness(dest, dest_size);
	packet->offset += dest_size;
}

void packet_read_varint(bedrock_packet *packet, int32_t *dest)
{
	int i;

	*dest = 0;

	if (packet->error)
		return;
	else if (packet->offset >= packet->buffer.length)
	{
		packet->error = ERROR_EAGAIN;
		return;
	}

	for (i = 0; packet->offset < packet->buffer.length; ++i)
	{
		unsigned const char *p = packet->buffer.data + packet->offset++;
		if (i <= 4)
			*dest |= (*p & 0x7F) << (7 * i);
		if (!(*p & 0x80))
			return;
	}

	packet->error = ERROR_EAGAIN;
}

void packet_read_varuint(bedrock_packet *packet, uint32_t *dest)
{
	packet_read_varint(packet, (int32_t *) dest);
}

void packet_read_string(bedrock_packet *packet, char *dest, size_t dest_size)
{
	uint32_t length;

	bedrock_assert(dest != NULL && dest_size > 0, goto error);

	packet_read_varuint(packet, &length);

	*dest = 0;

	if (packet->error)
		return;

	if (length >= dest_size)
	{
		packet->error = ERROR_EAGAIN;
		return;
	}

	/* Remember, this length is length in CHARACTERS */
	if (packet->offset + length > packet->buffer.length)
	{
		packet->error = ERROR_EAGAIN;
		return;
	}

	if (length > BEDROCK_MAX_STRING_LENGTH - 1)
	{
		packet->error = ERROR_INVALID_FORMAT;
		return;
	}

	bedrock_assert(length < dest_size, goto error);
	memcpy(dest, packet->buffer.data + packet->offset, length); // XXX UTF-8
	dest[length] = 0;

	packet->offset += length;
	return;

 error:
	packet->error = ERROR_UNKNOWN;
}

void packet_read_slot(bedrock_packet *packet, struct item_stack *stack)
{
	packet_read_int(packet, &stack->id, sizeof(stack->id));
	if (packet->error)
		return;
	if (stack->id != -1)
	{
		int16_t s;
		packet_read_int(packet, &stack->count, sizeof(stack->count));
		packet_read_int(packet, &stack->metadata, sizeof(stack->metadata));
		packet_read_int(packet, &s, sizeof(s));
		if (packet->error)
			return;
		if (s != -1)
			packet->offset += s; // Skip
	}
	else
	{
		stack->count = 0;
		stack->metadata = 0;
	}
}

void packet_pack(bedrock_packet *packet, const void *data, size_t size)
{
	bedrock_assert(packet != NULL && data != NULL, return);

	bedrock_buffer_append(&packet->buffer, data, size);
	packet->offset += size;
}

void packet_pack_int(bedrock_packet *packet, const void *data, size_t size)
{
	size_t old_len;

	bedrock_assert(packet != NULL && data != NULL, return);

	old_len = packet->buffer.length;
	packet_pack(packet, data, size);
	if (old_len + size == packet->buffer.length)
		convert_endianness(packet->buffer.data + old_len, size);
}

void packet_pack_varint(bedrock_packet *packet, int32_t data)
{
	packet_pack_varuint(packet, (uint32_t) data);
}

void packet_pack_varuint(bedrock_packet *packet, uint32_t data)
{
	unsigned char outbuf[5]; // Packed int32 can be at most 5 bytes

	bedrock_assert(packet != NULL, return);

	int size;
	if (data < (1 << 7))
		size = 1;
	else if (data < (1 << 14))
		size = 2;
	else if (data < (1 << 21))
		size = 3;
	else if (data < (1 << 28))
		size = 4;
	else
		size = 5;

	switch (size)
	{
		case 5:
			outbuf[4] = ((data >> 28) & 0xFF) | 0x80;
		case 4:
			outbuf[3] = ((data >> 21) & 0xFF) | 0x80;
		case 3:
			outbuf[2] = ((data >> 14) & 0xFF) | 0x80;
		case 2:
			outbuf[1] = ((data >> 7) & 0xFF) | 0x80;
		case 1:
			outbuf[0] = (data & 0xFF) | 0x80;
	}

	outbuf[size - 1] &= 0x7F;

	packet_pack(packet, outbuf, size);
}

void packet_pack_string(bedrock_packet *packet, const char *string)
{
	bedrock_assert(packet != NULL && string != NULL, return);
	packet_pack_string_len(packet, string, strlen(string));
}

void packet_pack_string_len(bedrock_packet *packet, const char *string, uint16_t len)
{
	bedrock_assert(packet != NULL && string != NULL, return);

	packet_pack_varuint(packet, len);

	bedrock_buffer_ensure_capacity(&packet->buffer, len);
	memcpy(packet->buffer.data + packet->buffer.length, string, len); // XXX UTF-8
	packet->buffer.length += len;
	packet->offset += len;
}

void packet_pack_slot(bedrock_packet *packet, struct item_stack *stack)
{
	packet_pack_int(packet, &stack->id, sizeof(stack->id));
	if (stack->id != -1)
	{
		int16_t s = -1;
		packet_pack_int(packet, &stack->count, sizeof(stack->count));
		packet_pack_int(packet, &stack->metadata, sizeof(stack->metadata));
		packet_pack_int(packet, &s, sizeof(s));
	}
}


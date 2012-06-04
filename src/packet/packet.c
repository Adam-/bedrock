#include "server/bedrock.h"
#include "packet/packet.h"
#include "util/endian.h"

#include "packet/packet_keep_alive.h"
#include "packet/packet_login_request.h"
#include "packet/packet_handshake.h"
#include "packet/packet_chat_message.h"
#include "packet/packet_player.h"
#include "packet/packet_position.h"
#include "packet/packet_player_look.h"
#include "packet/packet_held_item_change.h"
#include "packet/packet_position_and_look.h"
#include "packet/packet_entity_action.h"
#include "packet/packet_close_window.h"
#include "packet/packet_list_ping.h"
#include "packet/packet_disconnect.h"

static struct c2s_packet_handler
{
	uint8_t id;
	uint8_t len;
	uint8_t permission;
	uint8_t flags;
	int (*handler)(struct bedrock_client *, const unsigned char *buffer, size_t len);
} packet_handlers[] = {
	{KEEP_ALIVE,       5,  STATE_BURSTING | STATE_AUTHENTICATED,   HARD_SIZE, packet_keep_alive},
	{LOGIN_REQUEST,   20,  STATE_HANDSHAKING,     SOFT_SIZE, packet_login_request},
	{HANDSHAKE,        3,  STATE_UNAUTHENTICATED, SOFT_SIZE, packet_handshake},
	{CHAT_MESSAGE,     3,  STATE_AUTHENTICATED,   SOFT_SIZE, packet_chat_message},
	{PLAYER,           2,  STATE_BURSTING | STATE_AUTHENTICATED,   HARD_SIZE, packet_player},
	{PLAYER_POS,      34,  STATE_BURSTING | STATE_AUTHENTICATED,   HARD_SIZE, packet_position},
	{PLAYER_LOOK,     10,  STATE_AUTHENTICATED,   HARD_SIZE, packet_player_look},
	{PLAYER_POS_LOOK, 42,  STATE_BURSTING | STATE_AUTHENTICATED,   HARD_SIZE, packet_position_and_look},
	{HELD_ITEM_CHANGE, 3,  STATE_AUTHENTICATED,   HARD_SIZE, packet_held_item_change},
	{ENTITY_ACTION,    6,  STATE_AUTHENTICATED,   HARD_SIZE, packet_entity_action},
	{CLOSE_WINDOW,     2,  STATE_AUTHENTICATED,   HARD_SIZE, packet_close_window},
	{LIST_PING,        1,  STATE_UNAUTHENTICATED, HARD_SIZE, packet_list_ping},
	{DISCONNECT,       3,  STATE_ANY,             SOFT_SIZE, packet_disconnect},
};

static int packet_compare(const uint8_t *id, const struct c2s_packet_handler *handler)
{
	if (*id < handler->id)
		return -1;
	else if (*id > handler->id)
		return 1;
	return 0;
}

typedef int (*compare_func)(const void *, const void *);

/** Parse a packet. Returns -1 if the packet is invalid or unexpected, 0 if there is not
 * enough data yet, or the amount of data read from buffer.
 */
int packet_parse(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	uint8_t id = *buffer;
	int i;

	struct c2s_packet_handler *handler = bsearch(&id, packet_handlers, sizeof(packet_handlers) / sizeof(struct c2s_packet_handler), sizeof(struct c2s_packet_handler), (compare_func) packet_compare);
	if (handler == NULL)
	{
		bedrock_log(LEVEL_WARN, "packet: Unrecognized packet 0x%02x from %s", id, client_get_ip(client));
		client->in_buffer_len = 0;
		return -1;
	}

	if (len < handler->len)
		return 0;

	if ((handler->permission & client->authenticated) == 0)
	{
		bedrock_log(LEVEL_WARN, "packet: Unexpected packet 0x%02x from client %s - dropping client", id, client_get_ip(client));
		client_exit(client);
		return -1;
	}

	bedrock_log(LEVEL_PACKET_DEBUG, "packet: Got packet 0x%02x from %s (%s)", id, *client->name ? client->name : "(unknown)", client_get_ip(client));

	i = handler->handler(client, buffer, len);

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

void packet_read_int(const unsigned char *buffer, size_t buffer_size, size_t *offset, void *dest, size_t dest_size)
{
	if (*offset <= ERROR_UNKNOWN)
		return;
	else if (*offset + dest_size > buffer_size)
	{
		*offset = ERROR_EAGAIN;
		return;
	}

	memcpy(dest, buffer + *offset, dest_size);
	convert_endianness(dest, dest_size);
	*offset += dest_size;
}

void packet_read_string(const unsigned char *buffer, size_t buffer_size, size_t *offset, char *dest, size_t dest_size)
{
	uint16_t length, i, j;

	bedrock_assert(dest != NULL && dest_size > 0, goto error);

	packet_read_int(buffer, buffer_size, offset, &length, sizeof(length));

	*dest = 0;

	if (*offset <= ERROR_UNKNOWN)
		return;
	/* Remember, this length is length in CHARACTERS */
	else if (*offset + (length * 2) > buffer_size)
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
		dest[i] = *(buffer + *offset + j);
	dest[length] = 0;

	*offset += length * 2;
	return;

 error:
	*offset = ERROR_UNKNOWN;
}

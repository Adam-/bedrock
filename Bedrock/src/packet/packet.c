#include "server/bedrock.h"
#include "packet/packet.h"
#include "packet/packet_handler.h"
#include "util/endian.h"

static struct c2s_packet_handler
{
	uint8_t id;
	uint8_t len;
	uint8_t permission;
	uint8_t flags;
	int (*handler)(bedrock_client *, const unsigned char *buffer, size_t len);
} packet_handlers[] = {
	{KEEP_ALIVE,       5,  STATE_BURSTING,        HARD_SIZE, packet_keep_alive},
	{LOGIN_REQUEST,   20,  STATE_HANDSHAKING,             0, packet_login_request},
	{HANDSHAKE,        3,  STATE_UNAUTHENTICATED,         0, packet_handshake},
	{PLAYER,           2,  STATE_BURSTING,        HARD_SIZE, packet_player},
	{PLAYER_POS,      34,  STATE_BURSTING,        HARD_SIZE, packet_position},
	{PLAYER_POS_LOOK, 42,  STATE_BURSTING,        HARD_SIZE, packet_position_and_look},
};

static int packet_compare(const uint8_t *id, const struct c2s_packet_handler *handler)
{
	if (*id < handler->id)
		return -1;
	else if (*id > handler->id)
		return 1;
	return 0;
}

/** Parse a packet. Returns -1 if the packet is invalid or unexpected, 0 if there is not
 * enough data yet, or the amount of data read from buffer.
 */
int packet_parse(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	uint8_t id = *buffer;
	int i;

	struct c2s_packet_handler *handler = bsearch(&id, packet_handlers, sizeof(packet_handlers) / sizeof(struct c2s_packet_handler), sizeof(struct c2s_packet_handler), packet_compare);
	if (handler == NULL)
	{
		bedrock_log(LEVEL_WARN, "packet: Unrecognized packet 0x%02x from %s, dropping client", id, client_get_ip(client));
		client_exit(client);
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
			bedrock_log(LEVEL_WARN, "packet: Packet 0x%02x from client %s was handled improperly, expected %d and got %d - dropping client", id, handler->len, i, client_get_ip(client));
			client_exit(client);
			return;
		}

	return i;
}

void packet_read_int(const unsigned char *buffer, size_t buffer_size, size_t *offset, void *dest, size_t dest_size)
{
	if (*offset == ERROR_EAGAIN || *offset + dest_size > buffer_size)
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

	bedrock_assert(dest != NULL && dest_size > 0);

	packet_read_int(buffer, buffer_size, offset, &length, sizeof(length));

	*dest = 0;
	/* Remember, this length is length in CHARACTERS */
	if (*offset == ERROR_EAGAIN || *offset + (length * 2) > buffer_size)
	{
		*offset = ERROR_EAGAIN;
		return;
	}

	if (length >= dest_size)
	{
		*offset = ERROR_EAGAIN;
		return;
	}

	bedrock_assert(length < dest_size);

	for (i = 0, j = 1; i < length; ++i, j += 2)
		dest[i] = *(buffer + *offset + j);
	dest[length] = 0;

	*offset += length * 2;
}

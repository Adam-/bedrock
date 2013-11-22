#include "server/client.h"
#include "server/packet.h"
#include "config/config.h"
#include "util/string.h"
#include "packet/packet_disconnect.h"
#include "packet/packet_encryption_request.h"

enum
{
	STATUS = 1,
	LOGIN = 2
};

int packet_handshake(struct client *client, bedrock_packet *p)
{
	int32_t proto;
	char server_address[BEDROCK_MAX_STRING_LENGTH];
	uint16_t server_port;
	int32_t state;

	packet_read_varint(p, &proto);
	packet_read_string(p, server_address, sizeof(server_address));
	packet_read_int(p, &server_port, sizeof(server_port));
	packet_read_varint(p, &state);

	if (p->error)
		return p->error;

	if (proto != BEDROCK_PROTOCOL_VERSION)
	{
		packet_send_disconnect(client, "Incorrect version");
		return ERROR_OK;
	}

	switch (state)
	{
		case STATUS:
			client->state = STATE_STATUS;
			break;
		case LOGIN:
			client->state = STATE_LOGIN;
			break;
		default:
			packet_send_disconnect(client, "Unknown state");
			return ERROR_UNEXPECTED;
	}

	return ERROR_OK;
}

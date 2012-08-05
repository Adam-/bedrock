#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_login_request.h"

enum
{
	INITIAL_SPAWN,
	RESPAWN
};

int packet_client_status(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t b;

	packet_read_int(p, &offset, &b, sizeof(b));

	if (b == INITIAL_SPAWN)
	{
		if (client->authenticated != STATE_LOGGED_IN)
			return ERROR_UNEXPECTED;
		packet_send_login_request(client);
	}
	else if (b == RESPAWN)
		;
	else
		return ERROR_UNEXPECTED;
	
	return offset;
}


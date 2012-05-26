#include "server/client.h"
#include "packet/packet.h"

int packet_entity_action(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint32_t eid;
	uint8_t aid;

	packet_read_int(buffer, len, &offset, &eid, sizeof(eid));
	packet_read_int(buffer, len, &offset, &aid, sizeof(aid));

	if (eid != client->id)
		return ERROR_UNEXPECTED;

	client->action = aid;

	return offset;
}

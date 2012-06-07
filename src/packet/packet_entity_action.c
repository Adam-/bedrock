#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_entity_metadata.h"

enum
{
	METADATA_NONE = 0,
	METADATA_FIRE = 1 << 0,
	METADATA_CROUCHED = 1 << 1,
	METADATA_RIDING = 1 << 2,
	METADATA_SPRINTING = 1 << 3,
	METADATA_EATING = 1 << 4
};

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

	switch (client->action)
	{
		case ACTION_CROUCH:
		{
			uint8_t b = METADATA_CROUCHED;
			packet_send_entity_metadata(client, ENTITY_METADATA_INDEX_FLAGS, ENTITY_METADATA_TYPE_BYTE, &b, sizeof(b));
			break;
		}
		case ACTION_UNCROUCH:
		case ACTION_STOP_SPRINTING:
		{
			uint8_t b = METADATA_NONE;
			packet_send_entity_metadata(client, ENTITY_METADATA_INDEX_FLAGS, ENTITY_METADATA_TYPE_BYTE, &b, sizeof(b));
			break;
		}
		case ACTION_START_SPRINTING:
		{
			uint8_t b = METADATA_SPRINTING;
			packet_send_entity_metadata(client, ENTITY_METADATA_INDEX_FLAGS, ENTITY_METADATA_TYPE_BYTE, &b, sizeof(b));
			break;
		}
		case ACTION_LEAVE_BED:
		default:
			client->action = ACTION_NONE;
	}

	return offset;
}

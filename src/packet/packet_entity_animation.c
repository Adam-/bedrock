#include "server/client.h"
#include "server/packet.h"

enum
{
	ANIMATION_NONE = 0,
	ANIMATION_SWING_ARM = 1,
	ANIMATION_DAMAGE = 2,
	ANIMATION_LEAVE_BED = 3,
	ANIMATION_EAT = 5,
	ANIMATION_CROUCH = 104,
	ANIMATION_UNCROUCH = 105
};

int packet_entity_animation(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint32_t id;
	uint8_t anim;
	bedrock_node *node;

	packet_read_int(buffer, len, &offset, &id, sizeof(id));
	packet_read_int(buffer, len, &offset, &anim, sizeof(anim));

	if (id != client->id)
		return ERROR_UNEXPECTED;
	else if (anim != ANIMATION_SWING_ARM)
		return ERROR_UNEXPECTED;

	LIST_FOREACH(&client->players, node)
	{
		struct bedrock_client *c = node->data;

		packet_send_entity_animation(c, client, anim);
	}

	return offset;
}

void packet_send_entity_animation(struct bedrock_client *client, struct bedrock_client *target, uint8_t anim)
{
	client_send_header(client, ENTITY_ANIMATION);
	client_send_int(client, &target->id, sizeof(target->id));
	client_send_int(client, &anim, sizeof(anim));
}

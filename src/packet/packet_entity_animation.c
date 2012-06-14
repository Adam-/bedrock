#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_entity_animation.h"

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

int packet_entity_animation(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint32_t id;
	uint8_t anim;
	bedrock_node *node;

	packet_read_int(p, &offset, &id, sizeof(id));
	packet_read_int(p, &offset, &anim, sizeof(anim));

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
	bedrock_packet packet;

	packet_init(&packet, LOGIN_REQUEST);

	packet_pack_header(&packet, ENTITY_ANIMATION);
	packet_pack_int(&packet, &target->id, sizeof(target->id));
	packet_pack_int(&packet, &anim, sizeof(anim));

	client_send_packet(client, &packet);
}

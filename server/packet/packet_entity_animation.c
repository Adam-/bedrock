#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
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

int packet_entity_animation(struct client *client, bedrock_packet *p)
{
	uint32_t id;
	uint8_t anim;
	bedrock_node *node;

	packet_read_int(p, &id, sizeof(id));
	packet_read_int(p, &anim, sizeof(anim));

	if (p->error)
		return p->error;
	else if (id != client->id)
		return ERROR_UNEXPECTED;
	else if (anim != ANIMATION_SWING_ARM)
		return ERROR_UNEXPECTED;

	if (client->column != NULL)
		LIST_FOREACH(&client->column->players, node)
		{
			struct client *c = node->data;

			if (c == client)
				continue;

			packet_send_entity_animation(c, client, anim);
		}

	return ERROR_OK;
}

void packet_send_entity_animation(struct client *client, struct client *target, uint8_t anim)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_ANIMATION);

	packet_pack_varuint(&packet, target->id);
	packet_pack_int(&packet, &anim, sizeof(anim));

	client_send_packet(client, &packet);
}

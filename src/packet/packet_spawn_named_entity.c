#include "server/client.h"
#include "server/packet.h"
#include "blocks/items.h"
#include "nbt/nbt.h"
#include "packet/packet_entity_head_look.h"

void packet_send_spawn_named_entity(struct bedrock_client *client, struct bedrock_client *c)
{
	bedrock_packet packet;
	uint32_t abs_x, abs_y, abs_z;
	float yaw, pitch;
	int8_t y, p;
	nbt_tag *tag;
	struct bedrock_item *item;
	uint8_t b = 127;

	tag = client_get_inventory_tag(c, c->selected_slot);
	if (tag != NULL)
	{
		uint16_t *id = nbt_read(tag, TAG_SHORT, 1, "id");
		item = item_find_or_create(*id);
	}
	else
		item = item_find_or_create(ITEM_NONE);

	abs_x = *client_get_pos_x(c) * 32;
	abs_y = *client_get_pos_y(c) * 32;
	abs_z = *client_get_pos_z(c) * 32;

	yaw = *client_get_yaw(c);
	pitch = *client_get_pitch(c);

	y = (yaw / 360.0) * 256;
	p = (pitch / 360.0) * 256;

	packet_init(&packet, SPAWN_NAMED_ENTITY);

	packet_pack_header(&packet, SPAWN_NAMED_ENTITY);
	packet_pack_int(&packet, &c->id, sizeof(c->id));
	packet_pack_string(&packet, c->name);
	packet_pack_int(&packet, &abs_x, sizeof(abs_x));
	packet_pack_int(&packet, &abs_y, sizeof(abs_y));
	packet_pack_int(&packet, &abs_z, sizeof(abs_z));
	packet_pack_int(&packet, &y, sizeof(y));
	packet_pack_int(&packet, &p, sizeof(p));
	packet_pack_int(&packet, &item->id, sizeof(item->id));
	packet_pack_int(&packet, &b, sizeof(b));

	client_send_packet(client, &packet);

	packet_send_entity_head_look(client, c);
}

#include "server/client.h"
#include "server/packet.h"
#include "blocks/items.h"
#include "nbt/nbt.h"

void packet_send_spawn_named_entity(struct bedrock_client *client, struct bedrock_client *c)
{
	uint32_t abs_x, abs_y, abs_z;
	float yaw, pitch;
	int8_t y, p;
	nbt_tag *tag;
	struct bedrock_item *item;

	tag = client_get_inventory_tag(c, c->selected_slot);
	if (tag != NULL)
	{
		uint16_t *id = nbt_read(tag, TAG_SHORT, 1, "id");
		item = item_find_or_create(*id);
	}
	else
		item = item_find_or_create(0);

	abs_x = *client_get_pos_x(c) * 32;
	abs_y = *client_get_pos_y(c) * 32;
	abs_z = *client_get_pos_z(c) * 32;

	yaw = *client_get_yaw(c);
	pitch = *client_get_pitch(c);

	y = (yaw / 360.0) * 256;
	p = (pitch / 360.0) * 256;

	client_send_header(client, SPAWN_NAMED_ENTITY);
	client_send_int(client, &c->id, sizeof(c->id));
	client_send_string(client, c->name);
	client_send_int(client, &abs_x, sizeof(abs_x));
	client_send_int(client, &abs_y, sizeof(abs_y));
	client_send_int(client, &abs_z, sizeof(abs_z));
	client_send_int(client, &y, sizeof(y));
	client_send_int(client, &p, sizeof(p));
	client_send_int(client, &item->id, sizeof(item->id));
}

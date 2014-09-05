#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"
#include "blocks/items.h"
#include "nbt/nbt.h"

void packet_send_spawn_player(struct client *client, struct client *c)
{
	bedrock_packet packet;
	uint32_t abs_x, abs_y, abs_z;
	int8_t y, p;
	struct item_stack *weilded_item;
	struct item *item;

	weilded_item = &client->inventory[INVENTORY_HOTBAR_START + client->selected_slot];
	if (weilded_item->count)
		item = item_find_or_create(weilded_item->id);
	else
		item = item_find_or_create(ITEM_NONE);

	abs_x = c->x * 32;
	abs_y = c->y * 32;
	abs_z = c->z * 32;

	y = (c->yaw / 360.0) * 256;
	p = (c->pitch / 360.0) * 256;

	packet_init(&packet, SERVER_SPAWN_PLAYER);

	packet_pack_varuint(&packet, c->id);
	packet_pack_uuid(&packet, &c->uuid);
	packet_pack_int(&packet, abs_x);
	packet_pack_int(&packet, abs_y);
	packet_pack_int(&packet, abs_z);
	packet_pack_byte(&packet, y);
	packet_pack_byte(&packet, p);
	packet_pack_short(&packet, item->id);

	// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
	{
		uint8_t health_float = 102;
		float health = 10;
		uint8_t b = 127;
		packet_pack_byte(&packet, health_float);
		packet_pack_float(&packet, health);
		packet_pack_byte(&packet, b); // XXX metadata
	}

	client_send_packet(client, &packet);

	packet_send_entity_head_look(client, c);
}

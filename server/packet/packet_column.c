#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"
#include "server/column.h"
#include "nbt/nbt.h"
#include "util/compression.h"

#define COLUMN_BUFFER_SIZE 8192

static void packet_column_remove_players(struct client *client, struct column *column)
{
	bedrock_node *node;

	/* column goes away to client, so despawn any players in the column to them */
	LIST_FOREACH(&column->players, node)
	{
		struct client *cl = node->data;

		/* We only want players *in* this column not *near* this column */
		if (cl->column == column)
		{
			/* remove cl from clients view */
			packet_send_destroy_entity_player(client, cl);
		}
	}
}

static void packet_column_remove_items(struct client *client, struct column *column)
{
	bedrock_node *node;

	/* Remove items in this column */
	LIST_FOREACH(&column->items, node)
	{
		struct dropped_item *item = node->data;
		packet_spawn_object_item(client, item);
	}
}

void packet_send_column_empty(struct client *client, struct column *column)
{
	bedrock_packet packet;
	uint8_t b;
	uint16_t s;
	uint32_t i;

	packet_column_remove_players(client, column);
	packet_column_remove_items(client, column);

	packet_init(&packet, SERVER_MAP_COLUMN);

	packet_pack_int(&packet, nbt_read(column->data, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	packet_pack_int(&packet, nbt_read(column->data, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	packet_pack_int(&packet, &b, sizeof(b)); // Ground up continuous

	s = 0;
	packet_pack_int(&packet, &s, sizeof(s)); // Primary bitmap
	packet_pack_int(&packet, &s, sizeof(s)); // Add bitmap

	i = 0;
	packet_pack_int(&packet, &i, sizeof(i)); // Size

	client_send_packet(client, &packet);
}

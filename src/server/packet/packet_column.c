#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "nbt/nbt.h"
#include "packet/packet_destroy_entity.h"
#include "packet/packet_spawn_dropped_item.h"
#include "packet/packet_spawn_named_entity.h"
#include "util/compression.h"

#define COLUMN_BUFFER_SIZE 8192

static void packet_column_remove_players(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_node *node;

	/* This column is going away, find players in this column */
	LIST_FOREACH(&column->players, node)
	{
		struct bedrock_client *cl = node->data;

		/* We only want players *in* this column not *near* this column */
		if (cl->column == column)
		{
			/* Remove these clients from each other */
			packet_send_destroy_entity_player(client, cl);
			packet_send_destroy_entity_player(cl, client);
		}
	}
}

static void packet_column_remove_items(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_node *node;

	/* Remove items in this column */
	LIST_FOREACH(&column->items, node)
	{
		struct bedrock_dropped_item *item = node->data;
		packet_send_destroy_entity_dropped_item(client, item);
	}
}

void packet_send_column_empty(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_packet packet;
	uint8_t b;
	uint16_t s;
	uint32_t i;

	packet_column_remove_players(client, column);
	packet_column_remove_items(client, column);

	packet_init(&packet, SPAWN_DROPPED_ITEM);

	packet_pack_header(&packet, MAP_COLUMN);
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

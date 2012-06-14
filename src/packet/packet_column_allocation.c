#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "nbt/nbt.h"

void packet_send_column_allocation(struct bedrock_client *client, struct bedrock_column *column, bool allocate)
{
	bedrock_packet packet;
	uint8_t b;

	packet_init(&packet, MAP_COLUMN_ALLOCATION);

	packet_pack_header(&packet, MAP_COLUMN_ALLOCATION);
	packet_pack_int(&packet, &column->x, sizeof(column->x));
	packet_pack_int(&packet, &column->z, sizeof(column->z));
	b = allocate;
	packet_pack_int(&packet, &b, sizeof(b));

	client_send_packet(client, &packet);
}

#include "server/client.h"
#include "packet/packet.h"
#include "server/column.h"
#include "nbt/nbt.h"

void packet_send_column_allocation(struct bedrock_client *client, struct bedrock_column *column, bool allocate)
{
	uint8_t b;

	client_send_header(client, MAP_COLUMN_ALLOCATION);
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = allocate;
	client_send_int(client, &b, sizeof(b));
}

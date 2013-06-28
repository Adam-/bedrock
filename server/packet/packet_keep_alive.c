#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"

int packet_keep_alive(struct client *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	uint32_t id;

	packet_read_int(p, &offset, &id, sizeof(id));

	if (id == 0 || client->ping_id == 0)
		;
	else if (id != client->ping_id)
	{
		bedrock_log(LEVEL_INFO, "client: Dropping client %s (%s) due to invalid keepalive ID.", client->name, client_get_ip(client));
		packet_send_disconnect(client, "Ping timeout");
	}
	else
	{
		uint64_t second_diff = bedrock_time.tv_sec - client->ping_time_sent.tv_sec;
		int64_t nanosecond_diff = bedrock_time.tv_nsec - client->ping_time_sent.tv_nsec;

		if (nanosecond_diff < 0)
		{
			bedrock_assert(second_diff, ;);
			--second_diff;
			nanosecond_diff += 1000000000;
		}

		client->ping = second_diff * 1000 + nanosecond_diff / 1000000;

		if (client->authenticated & STATE_IN_GAME && client->authenticated & STATE_BURSTING)
		{
			client->authenticated &= ~STATE_BURSTING;
			bedrock_log(LEVEL_DEBUG, "client: End of burst for %s", client->name);
		}
	}

	return offset;
}

void packet_send_keep_alive(struct client *client, uint32_t id)
{
	bedrock_packet packet;

	packet_init(&packet, KEEP_ALIVE);

	packet_pack_header(&packet, KEEP_ALIVE);
	packet_pack_int(&packet, &id, sizeof(id));

	client_send_packet(client, &packet);

	client->ping_id = id;
	client->ping_time_sent = bedrock_time;
}

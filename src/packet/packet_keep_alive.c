#include "server/bedrock.h"
#include "server/client.h"
#include "packet/packet.h"
#include "packet/packet_disconnect.h"

int packet_keep_alive(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint32_t id;

	packet_read_int(buffer, len, &offset, &id, sizeof(id));

	if (id == 0)
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
	}

	return offset;
}

void packet_send_keep_alive(struct bedrock_client *client, uint32_t id)
{
	client_send_header(client, KEEP_ALIVE);
	client_send_int(client, &id, sizeof(id));

	client->ping_id = id;
	client->ping_time_sent = bedrock_time;
}

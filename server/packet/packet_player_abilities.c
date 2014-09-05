#include "server/client.h"
#include "server/packet.h"

int packet_player_abilities(struct client bedrock_attribute_unused *client, bedrock_packet *p)
{
	int8_t flags;
	float flying_speed;
	float walking_speed;

	packet_read_byte(p, &flags);
	packet_read_float(p, &flying_speed);
	packet_read_float(p, &walking_speed);

	return ERROR_OK;
}

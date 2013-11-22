#include "server/client.h"
#include "server/packet.h"

int packet_player_abilities(struct client bedrock_attribute_unused *client, bedrock_packet *p)
{
	uint8_t flags;
	float flying_speed;
	float walking_speed;

	packet_read_int(p, &flags, sizeof(flags));
	packet_read_int(p, &flying_speed, sizeof(flying_speed));
	packet_read_int(p, &walking_speed, sizeof(walking_speed));

	return ERROR_OK;
}

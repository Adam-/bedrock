#include "server/client.h"
#include "server/packet.h"

int packet_player_abilities(struct client bedrock_attribute_unused *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	uint8_t flags;
	float flying_speed;
	float walking_speed;

	packet_read_int(p, &offset, &flags, sizeof(flags));
	packet_read_int(p, &offset, &flying_speed, sizeof(flying_speed));
	packet_read_int(p, &offset, &walking_speed, sizeof(walking_speed));

	return offset;
}

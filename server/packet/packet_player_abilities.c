#include "server/client.h"
#include "server/packet.h"

int packet_player_abilities(struct client bedrock_attribute_unused *client, const bedrock_packet bedrock_attribute_unused *packet)
{
	return 4;
}

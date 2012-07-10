#include "server/bedrock.h"
#include "server/client.h"
#include "server/command.h"
#include "packet/packet_entity_teleport.h"

void command_teleport(struct bedrock_client *client, int bedrock_attribute_unused argc, const char **argv)
{
	struct bedrock_client *source = client_find(argv[1]), *target = client_find(argv[2]);

	if (source == NULL)
	{
		command_reply(client, "No such user: %s", argv[1]);
		return;
	}
	else if (target == NULL)
	{
		command_reply(client, "No such user: %s", argv[2]);
		return;
	}
	else if (source == target)
	{
		command_reply(client, "Can not teleport a player to themself");
		return;
	}
	else if (source->world != target->world)
	{
		command_reply(client, "Can not teleport users across dimensions (yet!)");
		return;
	}

	command_reply(client, "Teleporting %s to %s", source->name, target->name);

	client_update_position(source, *client_get_pos_x(target), *client_get_pos_y(target), *client_get_pos_z(target), *client_get_yaw(source), *client_get_pitch(source), target->stance, *client_get_on_ground(target));
	packet_send_entity_teleport(source, source);
}

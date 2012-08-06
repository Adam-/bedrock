#include "server/bedrock.h"
#include "server/client.h"
#include "server/command.h"
#include "packet/packet_entity_teleport.h"

#include <errno.h>

void command_teleport(struct bedrock_client *client, int argc, const char **argv)
{
	struct bedrock_client *source = client_find(argv[1]);

	if (source == NULL)
	{
		command_reply(client, "No such user: %s", argv[1]);
		return;
	}

	if (argc == 3)
	{
		struct bedrock_client *target = client_find(argv[2]);

		if (target == NULL)
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
	else if (argc == 5)
	{
		char *errptr;
		long long_x, long_y, long_z;

		errno = 0;
		errptr = NULL;
		long_x = strtol(argv[2], &errptr, 10);
		if (errno || *errptr)
		{
			command_reply(client, "Invalid X coordinate");
			return;
		}

		errno = 0;
		errptr = NULL;
		long_y = strtol(argv[3], &errptr, 10);
		if (errno || *errptr)
		{
			command_reply(client, "Invalid Y coordinate");
			return;
		}

		errno = 0;
		errptr = NULL;
		long_z = strtol(argv[4], &errptr, 10);
		if (errno || *errptr)
		{
			command_reply(client, "Invalid Z coordinate");
			return;
		}

		command_reply(client, "Teleporting %s to %d, %d, %d", source->name, long_x, long_y, long_z);

		client_update_position(source, long_x, long_y, long_z, *client_get_yaw(source), *client_get_pitch(source), source->stance, *client_get_on_ground(source));
		packet_send_entity_teleport(source, source);
	}
	else
	{
		command_reply(client, "Invalid usage, syntax: /%s <player1> [<player2> | <x> <y> <z>]", argv[0]);
		return;
	}
}

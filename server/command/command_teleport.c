#include "server/bedrock.h"
#include "server/client.h"
#include "server/command.h"
#include "packet/packet_entity_teleport.h"

#include <errno.h>

void command_teleport(struct command_source *source, int argc, const char **argv)
{
	struct client *user_source = client_find(argv[1]);

	if (source == NULL)
	{
		command_reply(source, "No such user: %s", argv[1]);
		return;
	}

	if (argc == 3)
	{
		struct client *target = client_find(argv[2]);

		if (target == NULL)
		{
			command_reply(source, "No such user: %s", argv[2]);
			return;
		}
		else if (user_source == target)
		{
			command_reply(source, "Can not teleport a player to themself");
			return;
		}
		else if (user_source->world != target->world)
		{
			command_reply(source, "Can not teleport users across dimensions (yet!)");
			return;
		}

		command_reply(source, "Teleporting %s to %s", user_source->name, target->name);

		client_update_position(user_source, *client_get_pos_x(target), *client_get_pos_y(target), *client_get_pos_z(target), *client_get_yaw(user_source), *client_get_pitch(user_source), target->stance, *client_get_on_ground(target));
		packet_send_entity_teleport(user_source, user_source);
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
			command_reply(source, "Invalid X coordinate");
			return;
		}

		errno = 0;
		errptr = NULL;
		long_y = strtol(argv[3], &errptr, 10);
		if (errno || *errptr)
		{
			command_reply(source, "Invalid Y coordinate");
			return;
		}

		errno = 0;
		errptr = NULL;
		long_z = strtol(argv[4], &errptr, 10);
		if (errno || *errptr)
		{
			command_reply(source, "Invalid Z coordinate");
			return;
		}

		command_reply(source, "Teleporting %s to %d, %d, %d", user_source->name, long_x, long_y, long_z);

		client_update_position(user_source, long_x, long_y, long_z, *client_get_yaw(user_source), *client_get_pitch(user_source), user_source->stance, *client_get_on_ground(user_source));
		packet_send_entity_teleport(user_source, user_source);
	}
	else
	{
		command_reply(source, "Invalid usage, syntax: /%s <player1> [<player2> | <x> <y> <z>]", argv[0]);
	}
}

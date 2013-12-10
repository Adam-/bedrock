#include "server/command.h"
#include "command/command_time.h"
#include "server/packets.h"

#include <errno.h>

static void update_time(struct command_source *source, struct world *world, long time)
{
	bedrock_node *node;

	command_reply(source, "Time in %s changed to %ld", world->name, time);
	world->time = time;
	
	LIST_FOREACH(&client_list, node)
	{
		struct client *c = node->data;

		if (c->world == world)
			packet_send_time(c);
	}
}

void command_time(struct command_source *source, int argc, const char **argv)
{
	struct world *world;

	if (source->user != NULL)
		world = source->user->world;
	else
	{
		if (world_list.count == 0)
		{
			command_reply(source, "There are no laoded worlds");
			return;
		}

		world = world_list.head->data;
	}

	if (argc == 3)
	{
		if (!strcasecmp(argv[1], "set"))
		{
			char *errptr = NULL;
			long time;

			errno = 0;
			time = strtol(argv[2], &errptr, 10);
			
			if (errno || *errptr)
			{
				command_reply(source, "Invalid time");
				return;
			}
			else
			{
				time %= 24000;
				update_time(source, world, time);
			}
		}
	}
	else if (argc == 2)
	{
		if (!strcasecmp(argv[1], "day") || !strcasecmp(argv[1], "noon"))
			update_time(source, world, 6000);
		else if (!strcasecmp(argv[1], "night") || !strcasecmp(argv[1], "midnight"))
			update_time(source, world, 18000);
		else if (!strcasecmp(argv[1], "dawn") || !strcasecmp(argv[1], "morning"))
			update_time(source, world, 24000);
		else if (!strcasecmp(argv[1], "dusk") || !strcasecmp(argv[1], "evening"))
			update_time(source, world, 12000);
	}
	else
	{
		bedrock_node *node;

		LIST_FOREACH(&world_list, node)
		{
			struct world *world = node->data;
			command_reply(source, "Current time in %s is %ld", world->name, world->time);
		}
	}
}

#include "server/command.h"
#include "command/command_time.h"
#include "packet/packet_time.h"

#include <errno.h>

static void update_time(struct bedrock_client *client, long time)
{
	bedrock_node *node;

	command_reply(client, "Time in %s changed to %ld", client->world->name, time);
	client->world->time = time;
	
	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if (c->world == client->world)
			packet_send_time(c);
	}
}

void command_time(struct bedrock_client *client, int argc, const char **argv)
{
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
				return;
			}
			else
			{
				time %= 24000;
				update_time(client, time);
			}
		}
	}
	else if (argc == 2)
	{
		if (!strcasecmp(argv[1], "day") || !strcasecmp(argv[1], "noon"))
			update_time(client, 6000);
		else if (!strcasecmp(argv[1], "night") || !strcasecmp(argv[1], "midnight"))
			update_time(client, 18000);
		else if (!strcasecmp(argv[1], "dawn") || !strcasecmp(argv[1], "morning"))
			update_time(client, 24000);
		else if (!strcasecmp(argv[1], "dusk") || !strcasecmp(argv[1], "evening"))
			update_time(client, 12000);
	}
	else
	{
		bedrock_node *node;

		LIST_FOREACH(&world_list, node)
		{
			struct bedrock_world *world = node->data;
			command_reply(client, "Current time in %s is %ld", world->name, world->time);
		}
	}
}

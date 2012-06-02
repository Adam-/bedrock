#include "server/command.h"

void command_help(struct bedrock_client *client, int argc, const char **argv)
{
	int i, count;

	command_reply(client, "Commands:");

	count = 0;
	for (i = 0; i < command_count; ++i)
	{
		struct bedrock_command *cmd = &commands[i];

		if (cmd->can_use(client))
		{
			command_reply(client, "  %-10s %-15s %s", cmd->name, cmd->syntax, cmd->desc);
			++count;
		}
	}

	command_reply(client, "%d commands available.", count);
}

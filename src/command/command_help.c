#include "server/command.h"

void command_help(struct bedrock_command_source *source, int bedrock_attribute_unused argc, const char bedrock_attribute_unused **argv)
{
	int i, count;

	command_reply(source, "Commands:");

	count = 0;
	for (i = 0; i < command_count; ++i)
	{
		struct bedrock_command *cmd = &commands[i];

		if (cmd->can_use(source, cmd))
		{
			command_reply(source, "%s%s%s - %s", cmd->name, *cmd->syntax ? " " : "", cmd->syntax, cmd->desc);
			++count;
		}
	}

	command_reply(source, "%d commands available.", count);
}

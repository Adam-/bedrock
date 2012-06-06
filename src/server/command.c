#include "server/bedrock.h"
#include "server/command.h"
#include "packet/packet_chat_message.h"
#include "command/command_fdlist.h"
#include "command/command_help.h"
#include "command/command_memory.h"
#include "command/command_players.h"
#include "command/command_shutdown.h"
#include "command/command_stats.h"
#include "command/command_uptime.h"
#include "command/command_version.h"

#define MAX_PARAMETERS 15

struct bedrock_command commands[] = {
	{"FDLIST",  "", "shows file descriptors", 0, 0, command_anyone, command_fdlist},
	{"HELP",    "", "shows this message",     0, 0, command_anyone, command_help},
	{"MEMORY",  "", "shows memory usage",     0, 0, command_anyone, command_memory},
	{"PLAYERS", "", "lists players online",   0, 0, command_anyone, command_players},
	{"SHUTDOWN", "", "shuts down the server", 0, 0, command_anyone, command_shutdown},
	{"STATS",   "", "shows statistics",       0, 0, command_anyone, command_stats},
	{"UPTIME",  "", "shows server uptime",    0, 0, command_anyone, command_uptime},
	{"VERSION", "", "shows server version",   0, 0, command_anyone, command_version}
};

int command_count = sizeof(commands) / sizeof(struct bedrock_command);

bool command_anyone(struct bedrock_client *client)
{
	return true;
}

static int command_compare(const char *name, const struct bedrock_command *command)
{
	return strcasecmp(name, command->name);
}

typedef int (*compare_func)(const void *, const void *);

struct bedrock_command *command_find(const char *command)
{
	return bsearch(command, commands, sizeof(commands) / sizeof(struct bedrock_command), sizeof(struct bedrock_command), (compare_func) command_compare);
}

void command_run(struct bedrock_client *client, const char *buf)
{
	char command_buf[BEDROCK_MAX_STRING_LENGTH];
	char *front, *end;
	struct bedrock_command *command;
	size_t argc = 0;
	char *argv[MAX_PARAMETERS];

	strncpy(command_buf, buf, sizeof(command_buf));

	front = command_buf;
	end = strchr(command_buf, ' ');
	if (end != NULL)
		*end++ = 0;

	command = command_find(front);
	if (command == NULL)
		return;

	if (command->max_parameters > MAX_PARAMETERS)
	{
		bedrock_log(LEVEL_INFO, "command: Warning: command %s allows more than the maximum parameters - %d", MAX_PARAMETERS);
		command->max_parameters = MAX_PARAMETERS;
	}

	while (front != NULL && argc < command->max_parameters)
	{
		argv[argc++] = front;

		front = end;
		if (front != NULL && argc != command->max_parameters)
		{
			end = strchr(front, ' ');
			if (end != NULL)
				*end++ = 0;
		}
	}

	if (argc < command->min_parameters)
	{
		command_reply(client, "Syntax error, syntax is: %s", command->syntax);
		return;
	}

	if (!command->can_use(client))
		return;

	command->handler(client, argc, (const char **) argv);
}

void command_reply(struct bedrock_client *client, const char *fmt, ...)
{
	va_list args;
	char message[BEDROCK_MAX_STRING_LENGTH];

	message[0] = (char) SPECIAL_CHAR;
	message[1] = COLOR_GREY;

	va_start(args, fmt);
	vsnprintf(message + 2, sizeof(message) - 2, fmt, args);
	va_end(args);

	packet_send_chat_message(client, "%s", message);
}

#include "server/bedrock.h"
#include "server/command.h"
#include "packet/packet_chat_message.h"
#include "command/command_fdlist.h"
#include "command/command_gamemode.h"
#include "command/command_help.h"
#include "command/command_me.h"
#include "command/command_memory.h"
#include "command/command_oper.h"
#include "command/command_players.h"
#include "command/command_shutdown.h"
#include "command/command_stats.h"
#include "command/command_time.h"
#include "command/command_teleport.h"
#include "command/command_uptime.h"
#include "command/command_version.h"

#define MAX_PARAMETERS 15

struct command commands[] = {
	{"FDLIST",   "",                                    "shows file descriptors",                         0, 0, command_use_oper,   command_fdlist},
	{"GAMEMODE", "<username> <mode>",                   "changes a user's gamemode",                      2, 2, command_use_oper,   command_gamemode},
	{"HELP",     "",                                    "shows this message",                             0, 0, command_use_anyone, command_help},
	{"ME",       "<action>",                            "performs an action",                             1, 1, command_use_anyone, command_me},
	{"MEMORY",   "",                                    "shows memory usage",                             0, 0, command_use_oper,   command_memory},
	{"OPER",     "<username> <password>",               "gain operator status",                           2, 2, command_use_anyone, command_oper},
	{"PLAYERS",  "",                                    "lists players online",                           0, 0, command_use_anyone, command_players},
	{"SHUTDOWN", "",                                    "shuts down the server",                          0, 0, command_use_oper,   command_shutdown},
	{"STATS",    "",                                    "shows statistics",                               0, 0, command_use_oper,   command_stats},
	{"TIME",     "[day|night|dawn|dusk|<number>]",      "Sets world time",                                0, 3, command_use_oper,   command_time},
	{"TP",       "<player1> [<player2> | <x> <y> <z>]", "teleport player1 to player2 or to coords x/y/z", 2, 4, command_use_oper,   command_teleport},
	{"UPTIME",   "",                                    "shows server uptime",                            0, 0, command_use_anyone, command_uptime},
	{"VERSION",  "",                                    "shows server version",                           0, 0, command_use_anyone, command_version}
};

int command_count = sizeof(commands) / sizeof(struct command);

bool command_use_anyone(struct command_source bedrock_attribute_unused *source, struct command bedrock_attribute_unused *command)
{
	return true;
}

bool command_use_oper(struct command_source *source, struct command *command)
{
	return source->user == NULL || oper_has_command(source->user->oper, command->name);
}

static int command_compare(const char *name, const struct command *command)
{
	return strcasecmp(name, command->name);
}

typedef int (*compare_func)(const void *, const void *);

struct command *command_find(const char *command)
{
	return bsearch(command, commands, sizeof(commands) / sizeof(struct command), sizeof(struct command), (compare_func) command_compare);
}

void command_run(struct command_source *source, const char *buf)
{
	char command_buf[BEDROCK_MAX_STRING_LENGTH];
	char *front, *end;
	struct command *command;
	int argc = 0;
	char *argv[MAX_PARAMETERS];

	strncpy(command_buf, buf, sizeof(command_buf));

	front = command_buf;
	end = strchr(command_buf, ' ');
	if (end != NULL)
		*end++ = 0;

	command = command_find(front);
	if (command == NULL || !command->can_use(source, command))
		return;

	if (command->max_parameters > MAX_PARAMETERS)
	{
		bedrock_log(LEVEL_INFO, "command: Warning: command %s allows more than the maximum parameters - %d", MAX_PARAMETERS);
		command->max_parameters = MAX_PARAMETERS;
	}

	while (front != NULL && argc - 1 < command->max_parameters)
	{
		argv[argc++] = front;

		front = end;
		if (front != NULL && argc < command->max_parameters)
		{
			end = strchr(front, ' ');
			if (end != NULL)
				*end++ = 0;
		}
	}

	if (argc - 1 < command->min_parameters)
	{
		command_reply(source, "Syntax error, syntax is: %s", command->syntax);
		return;
	}

	{
		int i;

		for (i = 0; i < argc; ++i)
			bedrock_log(LEVEL_DEBUG, "command: parameter %d: %s", i, argv[i]);
	}

	command->handler(source, argc, (const char **) argv);
}

void command_reply(struct command_source *source, const char *fmt, ...)
{
	va_list args;
	char message[BEDROCK_MAX_STRING_LENGTH];

	if (source->user != NULL)
	{
		message[0] = (char) SPECIAL_CHAR;
		message[1] = COLOR_GREY;

		va_start(args, fmt);
		vsnprintf(message + 2, sizeof(message) - 2, fmt, args);
		va_end(args);

		packet_send_chat_message(source->user, "%s", message);
	}
	else if (source->console != NULL)
	{
		va_start(args, fmt);
		vsnprintf(message, sizeof(message) - 1, fmt, args);
		va_end(args);

		strncat(message, "\n", 1);

		console_write(source->console, message);
	}
}

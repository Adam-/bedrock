#include "util/util.h"
#include "server/client.h"
#include "protocol/console.h"

struct command_source
{
	struct client *user;
	struct console_client *console;
};

struct command
{
	const char *name;
	const char *syntax;
	const char *desc;
	int min_parameters;
	int max_parameters;
	bool (*can_use)(struct command_source *client, struct command *command);
	void (*handler)(struct command_source *client, int argc, const char **argv);
};

extern struct command commands[];
extern int command_count;

extern bool command_use_anyone(struct command_source *source, struct command *command);
extern bool command_use_oper(struct command_source *source, struct command *command);

extern struct command *command_find(const char *command);
extern void command_run(struct command_source *source, const char *buf);

extern void command_reply(struct command_source *source, const char *fmt, ...) bedrock_printf(2);

#include "util/util.h"
#include "server/client.h"
#include "protocol/console.h"

struct bedrock_command_source
{
	struct bedrock_client *user;
	struct bedrock_console_client *console;
};

struct bedrock_command
{
	const char *name;
	const char *syntax;
	const char *desc;
	int min_parameters;
	int max_parameters;
	bool (*can_use)(struct bedrock_command_source *client, struct bedrock_command *command);
	void (*handler)(struct bedrock_command_source *client, int argc, const char **argv);
};

extern struct bedrock_command commands[];
extern int command_count;

extern bool command_use_anyone(struct bedrock_command_source *source, struct bedrock_command *command);
extern bool command_use_oper(struct bedrock_command_source *source, struct bedrock_command *command);

extern struct bedrock_command *command_find(const char *command);
extern void command_run(struct bedrock_command_source *source, const char *buf);

extern void command_reply(struct bedrock_command_source *source, const char *fmt, ...);

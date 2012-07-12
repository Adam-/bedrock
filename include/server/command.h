#include "util/util.h"
#include "server/client.h"

struct bedrock_command
{
	const char *name;
	const char *syntax;
	const char *desc;
	int min_parameters;
	int max_parameters;
	bool (*can_use)(struct bedrock_client *client, struct bedrock_command *command);
	void (*handler)(struct bedrock_client *client, int argc, const char **argv);
};

extern struct bedrock_command commands[];
extern int command_count;

extern bool command_use_anyone(struct bedrock_client *client, struct bedrock_command *command);
extern bool command_use_oper(struct bedrock_client *client, struct bedrock_command *command);

extern struct bedrock_command *command_find(const char *command);
extern void command_run(struct bedrock_client *client, const char *buf);

extern void command_reply(struct bedrock_client *client, const char *fmt, ...);

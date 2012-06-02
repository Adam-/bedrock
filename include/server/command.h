#include "util/util.h"
#include "server/client.h"

struct bedrock_command
{
	const char *name;
	const char *syntax;
	const char *desc;
	unsigned int min_parameters;
	unsigned int max_parameters;
	bool (*can_use)(struct bedrock_client *client);
	void (*handler)(struct bedrock_client *client, int argc, const char **argv);
};

extern struct bedrock_command commands[];
extern int command_count;

extern bool command_anyone(struct bedrock_client *client);

extern struct bedrock_command *command_find(const char *command);
extern void command_run(struct bedrock_client *client, const char *buf);

extern void command_reply(struct bedrock_client *client, const char *fmt, ...);

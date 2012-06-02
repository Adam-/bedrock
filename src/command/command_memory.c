#include "server/client.h"
#include "server/command.h"

void command_memory(struct bedrock_client *client, int argc, const char **argv)
{
	long double memory;
	int percent;

	if (!bedrock_memory)
		return;

	memory = bedrock_memory / 1024.0 / 1024.0;
	command_reply(client, "Total memory: %0.2LfMB", memory);

	memory = client_pool.size / 1024.0;
	percent = ((long double) client_pool.size / (long double) bedrock_memory) * 100.0;
	command_reply(client, "Client pool: %0.2LfKB (%0.2Lf%%)", memory, percent);
}

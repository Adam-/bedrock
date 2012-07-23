#include "server/client.h"
#include "server/command.h"
#include "server/column.h"

void command_memory(struct bedrock_client *client, int bedrock_attribute_unused argc, const char bedrock_attribute_unused **argv)
{
	long double memory;

	memory = __sync_or_and_fetch(&memory_size, 0);
	memory = memory / 1024.0 / 1024.0;

	command_reply(client, "Total memory: %0.2LfMB", memory);
}


#include "server/client.h"
#include "server/region.h"
#include "server/command.h"

static void show_memory_for_pool(struct bedrock_client *client, const char *pool_name, bedrock_memory_pool *pool)
{
	long double memory, percent;
	char *unit;

	memory = pool->size / 1024.0;
	unit = "KB";
	if (memory > 5 * 1024)
	{
		memory /= 1024.0;
		unit = "MB";

		if (memory > 2 * 1024)
		{
			memory /= 1024.0;
			unit = "GB";
		}
	}

	percent = ((long double) pool->size / (long double) bedrock_memory) * 100.0;
	command_reply(client, "%s: %0.2Lf%s (%0.2Lf%%)", pool_name, memory, unit, percent);
}

void command_memory(struct bedrock_client *client, int argc, const char **argv)
{
	long double memory;

	if (!bedrock_memory)
		return;

	memory = bedrock_memory / 1024.0 / 1024.0;
	command_reply(client, "Total memory: %0.2LfMB", memory);

	show_memory_for_pool(client, "Client pool", &client_pool);
	show_memory_for_pool(client, "World pool", &world_pool);
	show_memory_for_pool(client, "Region pool", &region_pool);
}

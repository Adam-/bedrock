#include "server/bedrock.h"
#include "server/command.h"
#include "server/column.h"

void command_stats(struct command_source *source, int bedrock_attribute_unused argc, const char bedrock_attribute_unused **argv)
{
	bedrock_node *node, *node2, *node3;
	int i;
	long long unsigned worlds = 0, regions = 0, columns = 0, chunks = 0, blocks = 0;
	long players = 0, connections = 0;

	LIST_FOREACH(&world_list, node)
	{
		struct world *world = node->data;

		++worlds;

		LIST_FOREACH(&world->regions, node2)
		{
			struct region *region = node2->data;

			++regions;

			LIST_FOREACH(&region->columns, node3)
			{
				struct column *column = node3->data;

				++columns;

				for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
				{
					struct chunk *chunk = column->chunks[i];

					if (chunk != NULL)
					{
						int j;

						++chunks;

						for (j = 0; j < BEDROCK_BLOCK_LENGTH; ++j)
							if (chunk->blocks[j])
								++blocks;
					}
				}
			}
		}
	}

	LIST_FOREACH(&client_list, node)
	{
		struct client *client = node->data;

		if (client->authenticated & STATE_IN_GAME)
			++players;
		++connections;
	}

	command_reply(source, "Worlds: %llu, Regions: %llu, Columns: %llu, Chunks: %llu, Blocks: %llu", worlds, regions, columns, chunks, blocks);
	command_reply(source, "Players: %ld, Console clients: %d, Connections: %ld", players, console_list.count, connections);

}

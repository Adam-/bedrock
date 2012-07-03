#include "server/bedrock.h"
#include "server/command.h"
#include "server/column.h"

void command_stats(struct bedrock_client *client, int bedrock_attribute_unused argc, const char bedrock_attribute_unused **argv)
{
	bedrock_node *node, *node2, *node3;
	int i;
	long long unsigned worlds = 0, regions = 0, columns = 0, chunks = 0, blocks = 0;
	long players = 0, connections = 0;

	LIST_FOREACH(&world_list, node)
	{
		struct bedrock_world *world = node->data;

		++worlds;

		LIST_FOREACH(&world->regions, node2)
		{
			struct bedrock_region *region = node2->data;

			++regions;

			bedrock_mutex_lock(&region->column_mutex);

			LIST_FOREACH(&region->columns, node3)
			{
				struct bedrock_column *column = node3->data;

				++columns;

				for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
				{
					struct bedrock_chunk *chunk = column->chunks[i];

					if (chunk != NULL)
					{
						int j;

						++chunks;

						chunk_decompress(chunk);

						for (j = 0; j < BEDROCK_BLOCK_LENGTH; ++j)
							if (chunk->blocks[j])
								++blocks;

						chunk_compress(chunk);
					}
				}
			}

			bedrock_mutex_unlock(&region->column_mutex);
		}
	}

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *client = node->data;

		if (client->authenticated == STATE_AUTHENTICATED)
			++players;
		++connections;
	}

	command_reply(client, "Worlds: %llu, Regions: %llu, Columns: %llu, Chunks: %llu, Blocks: %llu", worlds, regions, columns, chunks, blocks);
	command_reply(client, "Players: %ld, Connections: %ld", players, connections);

}

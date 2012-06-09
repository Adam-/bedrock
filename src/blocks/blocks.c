#include "server/bedrock.h"
#include "blocks/blocks.h"

struct bedrock_block bedrock_blocks[] = {
	{BLOCK_GRASS,   "Grass",   0.9, ITEM_FLAG_SHOVEL, ITEM_FLAG_NONE, ITEM_FLAG_NONE, NULL},
	{BLOCK_BEDROCK, "Bedrock", 0,   ITEM_FLAG_NONE,   ITEM_FLAG_NONE, ITEM_FLAG_NONE, NULL}
};

static int block_compare(const block_type *id, const struct bedrock_block *block)
{
	if (*id < block->id)
		return -1;
	else if (*id > block->id)
		return 1;
	return 0;
}

typedef int (*compare_func)(const void *, const void *);

struct bedrock_block *block_find(block_type id)
{
	return bsearch(&id, bedrock_blocks, sizeof(bedrock_blocks) / sizeof(struct bedrock_block), sizeof(struct bedrock_block), (compare_func) block_compare);
}

struct bedrock_block *block_find_or_create(block_type id)
{
	static struct bedrock_block b;
	struct bedrock_block *block = block_find(id);

	if (block == NULL)
	{
		bedrock_log(LEVEL_DEBUG, "block: Unrecognized block %d", id);

		b.id = id;
		b.name = "Unknown";
		b.hardness = 1;
		b.weakness = ITEM_FLAG_NONE;
		b.requirement = ITEM_FLAG_NONE;
		b.reap = ITEM_FLAG_NONE;
		b.on_mine = NULL;

		block = &b;
	}

	return block;
}

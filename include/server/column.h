#include "server/region.h"
#include "server/chunk.h"
#include "server/config.h"

struct bedrock_column
{
	struct bedrock_region *region;
	int32_t x;
	int32_t z;

	/* Data for this column. Note that the 'Sections' section is NOT here. We store it below. */
	nbt_tag *data;

	/* Chunks in this column */
	struct bedrock_chunk *chunks[BEDROCK_CHUNKS_PER_COLUMN];
};

extern struct bedrock_column *column_create(struct bedrock_region *region, nbt_tag *data);
extern void column_free(struct bedrock_column *column);

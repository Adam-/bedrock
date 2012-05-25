#include "server/region.h"

struct bedrock_column
{
	struct bedrock_region *region;
	int32_t x;
	int32_t z;
	nbt_tag *data;
};

extern void column_free(struct bedrock_column *column);

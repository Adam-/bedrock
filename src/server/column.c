#include "server/column.h"
#include "nbt/nbt.h"
#include "util/memory.h"

void column_free(struct bedrock_column *column)
{
	nbt_free(column->data);
	bedrock_free(column);
}

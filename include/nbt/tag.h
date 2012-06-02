#ifndef BEDROCK_NBT_TAG_H
#define BEDROCK_NBT_TAG_H

#include "util/list.h"
#include "util/memory.h"

#include <stdint.h>

typedef enum
{
	TAG_END,
	TAG_BYTE,
	TAG_SHORT,
	TAG_INT,
	TAG_LONG,
	TAG_FLOAT,
	TAG_DOUBLE,
	TAG_BYTE_ARRAY,
	TAG_STRING,
	TAG_LIST,
	TAG_COMPOUND,
	TAG_INT_ARRAY
} nbt_tag_type;

typedef struct _nbt_tag
{
	struct bedrock_memory_pool *pool;
	uint8_t type;
	char *name;
	struct _nbt_tag *owner;

	union
	{
		int8_t tag_byte;
		int16_t tag_short;
		int32_t tag_int;
		int64_t tag_long;
		float tag_float;
		double tag_double;

		struct nbt_tag_byte_array
		{
			int8_t *data;
			int32_t length;
		} tag_byte_array;

		char *tag_string;
		bedrock_list tag_list;
		bedrock_list tag_compound;

		struct nbt_tag_int_array
		{
			int32_t *data;
			int32_t length;
		} tag_int_array;
	}
	payload;

} nbt_tag;

#endif // BEDROCK_NBT_TAG_H

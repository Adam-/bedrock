#include "util/list.h"

typedef struct _nbt_tag
{
	uint8_t type;
	const char *name;

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
			int32_t length;
			char *data;
		} tag_byte_array;

		char *tag_string;
		bedrock_list tag_list;
		bedrock_list tag_compound;

		struct nbt_tag_int_array
		{
			int32_t length;
			int32_t *data;
		} tag_int_array;
	}
	payload;

} nbt_tag;

nbt_tag *nbt_parse(const unsigned char *data, size_t size);
void nbt_free(nbt_tag *tag);

#include "server/bedrock.h"
#include "nbt/nbt.h"
#include "util/memory.h"

uint16_t endian_test_i = 0x0001;
char *endian_test_c = (char *) &endian_test_i;

#define IS_LITTLE_ENDIAN *endian_test_c == 0x01

static bool read_bytes(unsigned char *dest, size_t dst_size, const unsigned char **src, size_t *src_size, bool swap)
{
	if (dst_size > *src_size)
	{
		bedrock_log(LEVEL_CRIT, "nbt: Out of data to read, %d > %d", dst_size, *src_size);
		return false;
	}

	memcpy(dest, *src, dst_size);

	*src += dst_size;
	*src_size -= dst_size;

	if (swap && IS_LITTLE_ENDIAN)
	{
		unsigned char *s = dest, *e = dest + dst_size - 1;

		for (; s < e; ++s, --e)
		{
			unsigned char t = *s;

			*s = *e;
			*e = t;
		}
	}

	return true;
}

static nbt_tag *read_named_tag(nbt_tag *tag, const unsigned char **data, size_t *size);

#define CHECK_RETURN(func, what) \
	do \
	{ \
		if (!(func)) \
			goto what; \
	} \
	while (0)

static nbt_tag *read_unnamed_tag(nbt_tag *tag, const unsigned char **data, size_t *size)
{
	switch (tag->type)
	{
		case TAG_END:
			break;
		case TAG_BYTE:
			CHECK_RETURN(read_bytes(&tag->payload.tag_byte, sizeof(tag->payload.tag_byte), data, size, true), error);
			break;
		case TAG_SHORT:
			CHECK_RETURN(read_bytes(&tag->payload.tag_short, sizeof(tag->payload.tag_short), data, size, true), error);
			break;
		case TAG_INT:
			CHECK_RETURN(read_bytes(&tag->payload.tag_int, sizeof(tag->payload.tag_int), data, size, true), error);
			break;
		case TAG_LONG:
			CHECK_RETURN(read_bytes(&tag->payload.tag_long, sizeof(tag->payload.tag_long), data, size, true), error);
			break;
		case TAG_FLOAT:
			CHECK_RETURN(read_bytes(&tag->payload.tag_float, sizeof(tag->payload.tag_float), data, size, true), error);
			break;
		case TAG_DOUBLE:
			CHECK_RETURN(read_bytes(&tag->payload.tag_double, sizeof(tag->payload.tag_double), data, size, true), error);
			break;
		case TAG_BYTE_ARRAY:
		{
			struct nbt_tag_byte_array *tba = &tag->payload.tag_byte_array;

			CHECK_RETURN(read_bytes(&tba->length, sizeof(tba->length), data, size, true), error);
			tba->data = bedrock_malloc(tba->length);
			CHECK_RETURN(read_bytes(tba->data, tba->length, data, size, false), error);
			break;
		}
		case TAG_STRING:
		{
			uint16_t str_len;

			CHECK_RETURN(read_bytes(&str_len, sizeof(str_len), data, size, true), error);

			tag->payload.tag_string = bedrock_malloc(str_len + 1);
			CHECK_RETURN(read_bytes(tag->payload.tag_string, str_len, data, size, false), error);
			tag->payload.tag_string[str_len] = 0;

			bedrock_log(LEVE_NBT_DEBUG, "nbt: Read string tag '%s'", tag->payload.tag_string);

			break;
		}
		case TAG_LIST:
		{
			int8_t tag_type;
			int32_t tag_length, i;

			CHECK_RETURN(read_bytes(&tag_type, sizeof(tag_type), data, size, true), error);
			CHECK_RETURN(read_bytes(&tag_length, sizeof(tag_length), data, size, true), error);

			for (i = 0; i < tag_length; ++i)
			{
				nbt_tag *nested_tag = bedrock_malloc(sizeof(nbt_tag));

				nested_tag->type = tag_type;
				nested_tag = read_unnamed_tag(nested_tag, data, size);

				if (nested_tag != NULL)
					bedrock_list_add(&tag->payload.tag_list, nested_tag);
			}
			break;
		}
		case TAG_COMPOUND:
		{
			while (true)
			{
				nbt_tag *nested_tag = bedrock_malloc(sizeof(nbt_tag));
				nested_tag = read_named_tag(nested_tag, data, size);

				if (nested_tag == NULL)
					break;

				bedrock_log(LEVE_NBT_DEBUG, "nbt: Read compound tag in '%s', named '%s'", tag->name, nested_tag->name);
				bedrock_list_add(&tag->payload.tag_compound, nested_tag);
			}

			break;
		}
		case TAG_INT_ARRAY:
		{
			struct nbt_tag_int_array *tia = &tag->payload.tag_int_array;

			CHECK_RETURN(read_bytes(&tia->length, sizeof(tia->length), data, size, true), error);
			tia->data = bedrock_malloc(sizeof(int32_t) * tia->length);
			CHECK_RETURN(read_bytes(tia->data, sizeof(int32_t) * tia->length, data, size, false), error);
			break;
		}
		default:
			bedrock_log(LEVEL_CRIT, "nbt: Unknown tag type - %d", tag->type);
			return NULL;
	}

	return tag;

 error:
	bedrock_log(LEVEL_CRIT, "nbt: Data corruption error on tag %s, type %d, offset %d", tag->name ? tag->name : "(unknown)", tag->type, *size);
	nbt_free(tag);
	return NULL;
}

static nbt_tag *read_named_tag(nbt_tag *tag, const unsigned char **data, size_t *size)
{
	uint16_t name_length;

	CHECK_RETURN(read_bytes(&tag->type, sizeof(tag->type), data, size, true), error);

	if (tag->type == TAG_END)
	{
		nbt_free(tag);
		return NULL;
	}

	CHECK_RETURN(read_bytes(&name_length, sizeof(name_length), data, size, true), error);

	tag->name = bedrock_malloc(name_length + 1);
	CHECK_RETURN(read_bytes(tag->name, name_length, data, size, false), error);
	tag->name[name_length] = 0;

	bedrock_log(LEVE_NBT_DEBUG, "nbt: Read named tag '%s' of length %d of type %d", tag->name ? tag->name : "(unknown)", name_length, tag->type);

	return read_unnamed_tag(tag, data, size);

 error:
	bedrock_log(LEVEL_CRIT, "nbt: Data corruption error on tag %s, type %d, offset %d", tag->name ? tag->name : "(unknown)", tag->type, *size);
	nbt_free(tag);
	return NULL;
}

nbt_tag *nbt_parse(const unsigned char *data, size_t size)
{
	nbt_tag *tag = bedrock_malloc(sizeof(nbt_tag));
	return read_named_tag(tag, &data, &size);
}

void nbt_free(nbt_tag *tag)
{
	bedrock_free(tag->name);

	switch (tag->type)
	{
		case TAG_BYTE_ARRAY:
			bedrock_free(tag->payload.tag_byte_array.data);
			break;
		case TAG_STRING:
			bedrock_free(tag->payload.tag_string);
			break;
		case TAG_LIST:
			tag->payload.tag_list.free = nbt_free;
			bedrock_list_clear(&tag->payload.tag_list);
			break;
		case TAG_COMPOUND:
			tag->payload.tag_compound.free = nbt_free;
			bedrock_list_clear(&tag->payload.tag_compound);
			break;
		case TAG_INT_ARRAY:
			bedrock_free(tag->payload.tag_int_array.data);
			break;
		default:
			break;
	}

	bedrock_free(tag);
}


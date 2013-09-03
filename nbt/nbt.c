#include "nbt/nbt.h"
#include "util/memory.h"
#include "util/endian.h"
#include "util/string.h"

static bool read_bytes(void *dest, size_t dst_size, const unsigned char **src, size_t *src_size, bool swap)
{
	if (dst_size > *src_size)
	{
		bedrock_log(LEVEL_CRIT, "nbt: Out of data to read, %d > %d", dst_size, *src_size);
		return false;
	}

	memcpy(dest, *src, dst_size);

	*src += dst_size;
	*src_size -= dst_size;

	if (swap)
		convert_endianness(dest, dst_size);

	return true;
}

static nbt_tag *read_named_tag(nbt_tag *tag, const unsigned char **data, size_t *size);
static void write_named_tag(bedrock_buffer *buffer, nbt_tag *tag);

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

			bedrock_log(LEVEL_NBT_DEBUG, "nbt: Read string tag '%s'", tag->payload.tag_string);

			break;
		}
		case TAG_LIST:
		{
			struct nbt_tag_list *tl = &tag->payload.tag_list;
			int32_t i;

			bedrock_assert((size_t) tl->length == tl->list.count, ;);

			CHECK_RETURN(read_bytes(&tl->type, sizeof(tl->type), data, size, true), error);
			CHECK_RETURN(read_bytes(&tl->length, sizeof(tl->length), data, size, true), error);

			for (i = 0; i < tl->length; ++i)
			{
				nbt_tag *nested_tag = bedrock_malloc(sizeof(nbt_tag));

				nested_tag->type = tl->type;
				nested_tag->owner = tag;
				nested_tag = read_unnamed_tag(nested_tag, data, size);

				if (nested_tag != NULL)
					bedrock_list_add(&tl->list, nested_tag);
			}
			break;
		}
		case TAG_COMPOUND:
		{
			while (true)
			{
				nbt_tag *nested_tag = bedrock_malloc(sizeof(nbt_tag));

				nested_tag->owner = tag;
				nested_tag = read_named_tag(nested_tag, data, size);

				if (nested_tag == NULL)
					break;

				bedrock_log(LEVEL_NBT_DEBUG, "nbt: Read compound tag in '%s', named '%s'", tag->name ? tag->name : "(unknown)", nested_tag->name);

				bedrock_list_add(&tag->payload.tag_compound, nested_tag);
			}

			break;
		}
		case TAG_INT_ARRAY:
		{
			struct nbt_tag_int_array *tia = &tag->payload.tag_int_array;
			int32_t i;

			CHECK_RETURN(read_bytes(&tia->length, sizeof(tia->length), data, size, true), error);
			tia->data = bedrock_malloc(sizeof(int32_t) * tia->length);
			CHECK_RETURN(read_bytes(tia->data, sizeof(int32_t) * tia->length, data, size, false), error);

			for (i = 0; i < tia->length; ++i)
				convert_endianness((unsigned char *) (tia->data + i), sizeof(int32_t));

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

	bedrock_log(LEVEL_NBT_DEBUG, "nbt: Read named tag '%s' of length %d of type %d", tag->name ? tag->name : "(unknown)", name_length, tag->type);

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

static void write_unnamed_tag(bedrock_buffer *buffer, nbt_tag *tag)
{
	switch (tag->type)
	{
		case TAG_BYTE:
		{
			int8_t b = tag->payload.tag_byte;
			bedrock_buffer_append(buffer, &b, sizeof(b));
			break;
		}
		case TAG_SHORT:
		{
			int16_t s = tag->payload.tag_short;
			convert_endianness((unsigned char *) &s, sizeof(s));
			bedrock_buffer_append(buffer, &s, sizeof(s));
			break;
		}
		case TAG_INT:
		{
			int32_t i = tag->payload.tag_int;
			convert_endianness((unsigned char *) &i, sizeof(i));
			bedrock_buffer_append(buffer, &i, sizeof(i));
			break;
		}
		case TAG_LONG:
		{
			int64_t l = tag->payload.tag_long;
			convert_endianness((unsigned char *) &l, sizeof(l));
			bedrock_buffer_append(buffer, &l, sizeof(l));
			break;
		}
		case TAG_FLOAT:
		{
			float f = tag->payload.tag_float;
			convert_endianness((unsigned char *) &f, sizeof(f));
			bedrock_buffer_append(buffer, &f, sizeof(f));
			break;
		}
		case TAG_DOUBLE:
		{
			double d = tag->payload.tag_double;
			convert_endianness((unsigned char *) &d, sizeof(d));
			bedrock_buffer_append(buffer, &d, sizeof(d));
			break;
		}
		case TAG_BYTE_ARRAY:
		{
			struct nbt_tag_byte_array *tba = &tag->payload.tag_byte_array;
			int32_t len = tba->length;

			convert_endianness((unsigned char *) &len, sizeof(len));
			bedrock_buffer_append(buffer, &len, sizeof(len));

			bedrock_buffer_append(buffer, tba->data, tba->length);
			break;
		}
		case TAG_STRING:
		{
			uint16_t str_len = strlen(tag->payload.tag_string);

			convert_endianness((unsigned char *) &str_len, sizeof(str_len));
			bedrock_buffer_append(buffer, (const unsigned char *) &str_len, sizeof(str_len));

			bedrock_buffer_append(buffer, tag->payload.tag_string, strlen(tag->payload.tag_string));
			break;
		}
		case TAG_LIST:
		{
			struct nbt_tag_list *tl = &tag->payload.tag_list;
			int32_t len = tl->length;
			bedrock_node *node;

			bedrock_assert((size_t) tl->length == tl->list.count, ;);

			bedrock_buffer_append(buffer, (const unsigned char *) &tl->type, sizeof(tl->type));

			convert_endianness((unsigned char *) &len, sizeof(len));
			bedrock_buffer_append(buffer, &len, sizeof(len));

			LIST_FOREACH(&tl->list, node)
			{
				nbt_tag *nested_tag = node->data;

				write_unnamed_tag(buffer, nested_tag);
			}
			break;
		}
		case TAG_COMPOUND:
		{
			bedrock_node *node;
			uint8_t t = TAG_END;

			LIST_FOREACH(&tag->payload.tag_compound, node)
			{
				nbt_tag *nested_tag = node->data;

				write_named_tag(buffer, nested_tag);
			}

			bedrock_buffer_append(buffer, &t, sizeof(t));

			break;
		}
		case TAG_INT_ARRAY:
		{
			struct nbt_tag_int_array *tia = &tag->payload.tag_int_array;
			int32_t i, len = tia->length;
			int32_t *b;
			size_t b_len;

			convert_endianness((unsigned char *) &len, sizeof(len));
			bedrock_buffer_append(buffer, &len, sizeof(len));

			b_len = buffer->length;
			bedrock_buffer_append(buffer, tia->data, sizeof(int32_t) * tia->length);

			b = (int32_t *) (buffer->data + b_len);
			for (i = 0; i < tia->length; ++i)
				convert_endianness((unsigned char *) (b + i), sizeof(int32_t));
			break;
		}
		default:
			bedrock_log(LEVEL_CRIT, "nbt: Unknown tag type - %d", tag->type);
	}
}

static void write_named_tag(bedrock_buffer *buffer, nbt_tag *tag)
{
	uint8_t type;
	uint16_t name_length;

	bedrock_assert(tag->type != TAG_END, ;);

	type = tag->type;
	bedrock_buffer_append(buffer, &type, sizeof(type));

	name_length = strlen(tag->name);
	convert_endianness((unsigned char *) &name_length, sizeof(name_length));
	bedrock_buffer_append(buffer, &name_length, sizeof(name_length));

	bedrock_buffer_append(buffer, tag->name, strlen(tag->name));

	write_unnamed_tag(buffer, tag);
}

bedrock_buffer *nbt_write(nbt_tag *tag)
{
	bedrock_buffer *buffer;

	bedrock_assert(tag->type == TAG_COMPOUND, return NULL);

	buffer = bedrock_buffer_create("nbt write", NULL, 0, BEDROCK_BUFFER_DEFAULT_SIZE);
	write_named_tag(buffer, tag);
	return buffer;
}

void nbt_free(nbt_tag *tag)
{
	bedrock_assert(tag != NULL, return);
	bedrock_assert(tag->type == TAG_COMPOUND || tag->owner != NULL, ;);

	if (tag->owner != NULL)
	{
		bedrock_assert(tag->owner->type == TAG_LIST || tag->owner->type == TAG_COMPOUND, ;);
		if (tag->owner->type == TAG_COMPOUND)
			bedrock_list_del(&tag->owner->payload.tag_compound, tag);
		else
		{
			bedrock_list_del(&tag->owner->payload.tag_list.list, tag);
			--tag->owner->payload.tag_list.length;
		}
	}

	bedrock_free(tag->name);
	nbt_clear(tag);
	bedrock_free(tag);
}

void nbt_clear(nbt_tag *tag)
{
	switch (tag->type)
	{
		case TAG_BYTE_ARRAY:
			bedrock_free(tag->payload.tag_byte_array.data);
			break;
		case TAG_STRING:
			bedrock_free(tag->payload.tag_string);
			break;
		case TAG_LIST:
			tag->payload.tag_list.list.free = (bedrock_free_func) nbt_free;
			bedrock_list_clear(&tag->payload.tag_list.list);
			break;
		case TAG_COMPOUND:
			tag->payload.tag_compound.free = (bedrock_free_func) nbt_free;
			bedrock_list_clear(&tag->payload.tag_compound);
			break;
		case TAG_INT_ARRAY:
			bedrock_free(tag->payload.tag_int_array.data);
			break;
		default:
			memset(&tag->payload, 0, sizeof(tag->payload));
			break;
	}
}

static nbt_tag *nbt_get_from_valist(nbt_tag *tag, size_t size, va_list list)
{
	size_t i;
	nbt_tag *t = tag;

	bedrock_assert(tag != NULL, return NULL);

	for (i = 0; i < size; ++i)
	{
		bedrock_node *n;
		bool found = false;

		bedrock_assert(t->type == TAG_LIST || t->type == TAG_COMPOUND, return NULL);

		if (t->type == TAG_LIST)
		{
			int pos = va_arg(list, int);
			int count = 0;

			LIST_FOREACH(&t->payload.tag_list.list, n)
			{
				nbt_tag *t2 = n->data;

				if (count++ == pos)
				{
					t = t2;
					found = true;
					break;
				}
			}
		}
		else if (t->type == TAG_COMPOUND)
		{
			const char *name = va_arg(list, const char *);

			LIST_FOREACH(&t->payload.tag_compound, n)
			{
				nbt_tag *t2 = n->data;

				if (!strcmp(t2->name, name))
				{
					t = t2;
					found = true;
					break;
				}
			}
		}

		if (found == false)
			return NULL;
	}

	return t;
}

nbt_tag *nbt_get(nbt_tag *tag, nbt_tag_type type, size_t size, ...)
{
	va_list list;
	va_start(list, size);

	tag = nbt_get_from_valist(tag, size, list);

	va_end(list);

	bedrock_assert(tag != NULL && tag->type == type, return NULL);
	return tag;
}

void nbt_copy(nbt_tag *tag, nbt_tag_type type, void *dest, size_t dest_size, size_t size, ...)
{
	va_list list;
	va_start(list, size);

	tag = nbt_get_from_valist(tag, size, list);
	bedrock_assert(tag != NULL && tag->type == type, goto error);

	memcpy(dest, &tag->payload, dest_size);

 error:
	va_end(list);
}

void *nbt_read(nbt_tag *tag, nbt_tag_type type, size_t size, ...)
{
	va_list list;
	va_start(list, size);

	tag = nbt_get_from_valist(tag, size, list);

	va_end(list);

	bedrock_assert(tag != NULL && tag->type == type, return NULL);

	return &tag->payload;
}

const char *nbt_read_string(nbt_tag *tag, size_t size, ...)
{
	va_list list;
	va_start(list, size);

	tag = nbt_get_from_valist(tag, size, list);

	va_end(list);

	bedrock_assert(tag->type == TAG_STRING, return NULL);

	return tag->payload.tag_string;
}

void nbt_set(nbt_tag *tag, nbt_tag_type type, const void *src, size_t src_size, size_t size, ...)
{
	va_list list;
	va_start(list, size);

	tag = nbt_get_from_valist(tag, size, list);

	va_end(list);

	bedrock_assert(tag != NULL && tag->type == type, return);

	memcpy(&tag->payload, src, src_size);
}

nbt_tag *nbt_add(nbt_tag *tag, nbt_tag_type type, const char *name, const void *src, size_t src_size)
{
	nbt_tag *c;

	bedrock_assert(tag == NULL || (tag->type == TAG_LIST || tag->type == TAG_COMPOUND), return NULL);

	c = bedrock_malloc(sizeof(nbt_tag));
	if (name != NULL)
		c->name = bedrock_strdup(name);
	c->owner = tag;
	c->type = type;

	switch (type)
	{
		case TAG_BYTE:
		case TAG_SHORT:
		case TAG_INT:
		case TAG_LONG:
		case TAG_FLOAT:
		case TAG_DOUBLE:
			memcpy(&c->payload, src, src_size);
			break;
		case TAG_BYTE_ARRAY:
		{
			struct nbt_tag_byte_array *tba = &c->payload.tag_byte_array;

			tba->data = bedrock_malloc(src_size);
			memcpy(tba->data, src, src_size);
			tba->length = src_size;
			break;
		}
		case TAG_STRING:
		{
			c->payload.tag_string = bedrock_malloc(src_size + 1);
			bedrock_strncpy(c->payload.tag_string, src, src_size + 1);
			break;
		}
		case TAG_LIST:
		case TAG_COMPOUND:
		{
			bedrock_assert(src == NULL, ;);
			break;
		}
		case TAG_INT_ARRAY:
		{
			struct nbt_tag_int_array *tia = &c->payload.tag_int_array;

			tia->data = bedrock_malloc(src_size);
			memcpy(tia->data, src, src_size);
			tia->length = src_size;
			break;
		}
		case TAG_END:
			break;
	}

	if (tag == NULL)
		;
	else if (tag->type == TAG_LIST)
	{
		struct nbt_tag_list *tl = &tag->payload.tag_list;

		bedrock_assert(type == TAG_END || tl->type == TAG_END || tl->type == type, ;);
		tl->type = type;

		++tl->length;
		bedrock_list_add(&tl->list, c);

		bedrock_assert((size_t) tl->length == tl->list.count, ;);
	}
	else //if (tag->type == TAG_COMPOUND)
		bedrock_list_add(&tag->payload.tag_compound, c);

	return c;
}


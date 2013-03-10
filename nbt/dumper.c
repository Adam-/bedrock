#include "nbt/nbt.h"

static void recursive_dump_tag(FILE *out, nbt_tag *t, int level)
{
	int r;
	const char *name = t->name ? t->name : "";

	for (r = 0; r < level; ++r)
		fprintf(out, "	");

	switch (t->type)
	{
		case TAG_END:
			break;
		case TAG_BYTE:
			fprintf(out, "TAG_Byte(%s): %d\n", name, t->payload.tag_byte);
			break;
		case TAG_SHORT:
			fprintf(out, "TAG_Short(%s): %d\n", name, t->payload.tag_short);
			break;
		case TAG_INT:
			fprintf(out, "TAG_Int(%s): %d\n", name, t->payload.tag_int);
			break;
		case TAG_LONG:
			fprintf(out, "TAG_Long(%s): %ld\n", name, t->payload.tag_long);
			break;
		case TAG_FLOAT:
			fprintf(out, "TAG_Float(%s): %f\n", name, t->payload.tag_float);
			break;
		case TAG_DOUBLE:
			fprintf(out, "TAG_Double(%s): %f\n", name, t->payload.tag_double);
			break;
		case TAG_BYTE_ARRAY:
		{
			struct nbt_tag_byte_array *tba = &t->payload.tag_byte_array;
			fprintf(out, "TAG_ByteArray(%s): length %d\n", name, tba->length);
			break;
		}
		case TAG_STRING:
		{
			fprintf(out, "TAG_String(%s): %s\n", name, t->payload.tag_string);
			break;
		}
		case TAG_LIST:
		{
			bedrock_node *n;
			struct nbt_tag_list *tl = &t->payload.tag_list;

			bedrock_assert((size_t) tl->length == tl->list.count, ;);

			fprintf(out, "TAG_List(%s): length %d\n", name, tl->length);
			for (r = 0; r < level; ++r)
				fprintf(out, "	");
			fprintf(out, "{\n");

			LIST_FOREACH(&t->payload.tag_list.list, n)
			{
				nbt_tag *t2 = n->data;
				recursive_dump_tag(out, t2, level + 1);
			}

			for (r = 0; r < level; ++r)
				fprintf(out, "	");
			fprintf(out, "}\n");
			break;
		}
		case TAG_COMPOUND:
		{
			bedrock_node *n;

			fprintf(out, "TAG_Compound(%s): length %ld\n", name, t->payload.tag_compound.count);
			for (r = 0; r < level; ++r)
				fprintf(out, "	");
			fprintf(out, "{\n");

			LIST_FOREACH(&t->payload.tag_compound, n)
			{
				nbt_tag *t2 = n->data;
				recursive_dump_tag(out, t2, level + 1);
			}

			for (r = 0; r < level; ++r)
				fprintf(out, "	");
			fprintf(out, "}\n");
			break;
		}
		case TAG_INT_ARRAY:
		{
			struct nbt_tag_int_array *tia = &t->payload.tag_int_array;
			fprintf(out, "TAG_IntArray(%s): length: %d\n", name, tia->length);
			break;
		}
		default:
			break;
	}
}

void nbt_ascii_dump(FILE *out, nbt_tag *tag)
{
	recursive_dump_tag(out, tag, 0);
}


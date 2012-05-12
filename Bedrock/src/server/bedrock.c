#include "server/bedrock.h"
#include "io/io.h"

void bedrock_log(bedrock_log_level level, const char *msg, ...)
{
	va_list args;
	char buffer[512];

	va_start(args, msg);
	vsnprintf(buffer, sizeof(buffer), msg, args);
	va_end(args);

	fprintf(stdout, "%s\n", buffer);
}

bool bedrock_running = true;

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include "nbt/nbt.h"
#include <errno.h>
#include <fcntl.h>
//#include "util/list.h"
#include "compression/compression.h"

static void dump_tag(nbt_tag *t)
{
	if (t->name)
		printf("Tag name %s, type %d, ", t->name, t->type);
	else
		printf("NO tag name, type %d\n", t->type);

	switch (t->type)
	{
		case TAG_END:
			break;
		case TAG_BYTE:
			printf("Byte: %d\n", t->payload.tag_byte);
			break;
		case TAG_SHORT:
			printf("Short: %d\n", t->payload.tag_short);
			break;
		case TAG_INT:
			printf("Int: %d\n", t->payload.tag_int);
			break;
		case TAG_LONG:
			printf("Long: %ld\n", t->payload.tag_long);
			break;
		case TAG_FLOAT:
			printf("Float: %f\n", t->payload.tag_float);
			break;
		case TAG_DOUBLE:
			printf("Double: %f\n", t->payload.tag_double);
			break;
		case TAG_BYTE_ARRAY:
		{
			struct nbt_tag_byte_array *tba = &t->payload.tag_byte_array;
			int32_t i;

			//for (i = 0; i < tba->length; ++i)
				//printf("%d:%d ", i, tba->data[i]);
			printf("Got a byte array\n");
			break;
		}
		case TAG_STRING:
		{
			printf("String: %s\n", t->payload.tag_string);
			break;
		}
		case TAG_LIST:
		{
			printf("Entering into list\n");
			bedrock_node *n;
			LIST_FOREACH(&t->payload.tag_list, n)
			{
				nbt_tag *t2 = n->data;
				dump_tag(t2);
			}
			break;
		}
		case TAG_COMPOUND:
		{
			printf("Entering into compound\n");
			bedrock_node *n;
			LIST_FOREACH(&t->payload.tag_list, n)
			{
				nbt_tag *t2 = n->data;
				dump_tag(t2);
			}
			break;
		}
		case TAG_INT_ARRAY:
		{
			struct nbt_tag_int_array *tia = &t->payload.tag_int_array;
			int32_t i;
			for (i = 0; i < tia->length; ++i)
				printf("  Int array %d: %d\n", i, tia->data[i]);
			break;
		}
		default:
			break;
	}
}

int main(int argc, char **argv)
{
	struct stat sb;
	int fd = open("/home/adam/cNBT/r.0.0.mca", O_RDONLY);
	assert(fd >= 0);
	assert(fstat(fd, &sb) == 0);
	char *p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	assert(p != MAP_FAILED);
	close(fd);

	//compression_buffer *cb = compression_decompress(p, sb.st_size);
	//munmap(p, sb.st_size);

	//assert(cb);

	int *ptr = p;
	int moo;
	for (moo = 0; moo < 1024; ++moo, ++ptr)
	{
		int copy = *ptr;
		copy = copy << 24 | (copy & 0xFF00) << 8 | (copy & 0xFF0000) >> 8 | copy >> 24;
		//rintf("%d\n", copy);

//		printf("used %d, offset * 4096 %d\n", copy & 0xFF, copy >> 8);
		char *p2 = p + ((copy >> 8) * 4096);
		int *ptr2 = p2;
		unsigned int len = *ptr2;
		len = len << 24 | (len & 0xFF00) << 8 | (len & 0xFF0000) >> 8 | len >> 24;
		p2 += 4;

		char type = *p2++;

		printf("TYPE: %d, len %d\n", type, len);
		compression_buffer *cb = compression_decompress(p2, len);

		nbt_tag *t = nbt_parse(cb->data, cb->length);
		assert(t);
		dump_tag(t);

		exit(0);
	}

	//for (moo = 0; moo < 1024; ++moo, ++ptr); // modified?

	exit(0);

	/*nbt_tag *t = nbt_parse(cb->data, cb->length);
	assert(t);
	dump_tag(t);
	compression_free_buffer(cb);
	printf("DONE\n");*/

	/*bedrock_io_init();
	init_listener();

	while (bedrock_running)
	{
		bedrock_io_process();
	}

	bedrock_io_shutdown();*/

	return 0;
}

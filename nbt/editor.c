#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "server/config/hard.h"
#include "util/compression.h"
#include "util/file.h"
#include "util/endian.h"
#include "util/string.h"
#include "nbt/nbt.h"
#include "dumper.h"

static bool pretend, debug, column;
static int column_x, column_z;
static const char *file_name;

static int fd;
static int ac;
static char **av;

void bedrock_log(bedrock_log_level level, const char *msg, ...)
{
	if (level == LEVEL_CRIT)
		;
	else if (level != LEVEL_NBT_DEBUG || !debug)
		return;

	va_list args;
	char buffer[512];

	va_start(args, msg);
	vsnprintf(buffer, sizeof(buffer) - 1, msg, args);
	va_end(args);

	printf("%s\n", buffer);
}

static void pack_int(const char *src, void **ptr, size_t *sz)
{
	static int i;
	i = atoi(src);
	*ptr = &i;
	*sz = sizeof(i);
}

static struct tag_info
{
	const char *name;
	nbt_tag_type type;
	void (*pack)(const char *src, void **ptr, size_t *sz);
} tag_infos[] = {
	{"INT", TAG_INT, pack_int},
	{"COMPOUND", TAG_COMPOUND, NULL},
	{"END", TAG_END, NULL},
};

static struct tag_info *tag_info_find(const char *name)
{
	unsigned i;
	for (i = 0; i < (sizeof(tag_infos) / sizeof(struct tag_info)); ++i)
		if (!strcmp(name, tag_infos[i].name))
			return &tag_infos[i];
	return NULL;
}

static void parse_args(int argc, char **argv)
{
	int c;

	struct option options[] = {
		{"column", required_argument, NULL, 'c'},
		{"debug", no_argument, NULL, 'd'},
		{"pretend", no_argument, NULL, 'p'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "c:dp", options, NULL)) != -1)
	{
		switch (c)
		{
			case 'c':
			{
				const char *x = optarg, *z;
				char *p = strchr(optarg, ',');
				if (!p)
				{
					fprintf(stderr, "Syntax: --column x,z\n");
					exit(-1);
				}
				*p++ = 0;
				z = p;

				column = true;
				column_x = atoi(x);
				column_z = atoi(z);
				break;
			}
			case 'd':
				debug = true;
				break;
			case 'p':
				pretend = true;
				break;
			case '?':
			default:
				break;
		}
	}

	if (optind >= argc)
	{
		fprintf(stderr, "Usage: %s file [options]\n", argv[0]);
		exit(-1);
	}

	file_name = argv[optind];

	ac = argc - optind - 1;
	av = argv + optind + 1;
}

static void offset_get(const unsigned char *contents, size_t sz, off_t *off, size_t *off_size)
{
	int32_t x, z;
	off_t offset;
	uint32_t read_offset, compressed_len;

	*off = 0;
	*off_size = sz;

	if (!column)
		return;

	x = column_x % BEDROCK_COLUMNS_PER_REGION;
	if (x < 0)
		x = BEDROCK_COLUMNS_PER_REGION - abs(x);

	z = column_z % BEDROCK_COLUMNS_PER_REGION;
	if (z < 0)
		z = BEDROCK_COLUMNS_PER_REGION - abs(z);

	offset = x + z * BEDROCK_COLUMNS_PER_REGION;
	offset *= sizeof(int32_t);

	bedrock_assert(offset + sizeof(read_offset) <= sz, return);
	memcpy(&read_offset, contents + offset, sizeof(read_offset));
	convert_endianness((unsigned char *) &read_offset, sizeof(read_offset));

	if (read_offset == 0)
		return;
	
	/* First 3 bytes are read offset, 4th byte is the length of the chunk */
	read_offset >>= 8;
	read_offset *= BEDROCK_REGION_SECTOR_SIZE;

	/* Get the length of the compressed data */
	bedrock_assert(read_offset + sizeof(compressed_len) <= sz, return);
	memcpy(&compressed_len, contents + read_offset, sizeof(compressed_len));
	convert_endianness((unsigned char *) &compressed_len, sizeof(compressed_len));

	/* The next byte is the compression type, which we assume */

	*off = read_offset + sizeof(compressed_len) + 1;
	*off_size = compressed_len;
}

static nbt_tag *get_contents_of(int f)
{
	unsigned char *contents;
	size_t sz;
	off_t offset;
	size_t offset_size;
	compression_buffer *buffer;
	nbt_tag *tag;

	contents = bedrock_file_read(f, &sz);
	if (contents == NULL)
	{
		fprintf(stderr, "Unable to read contents of %s\n", file_name);
		return NULL;
	}

	offset_get(contents, sz, &offset, &offset_size);

	buffer = compression_decompress(1024, contents + offset, offset_size);
	bedrock_free(contents);

	tag = nbt_parse(buffer->buffer->data, buffer->buffer->length);
	compression_decompress_end(buffer);
	
	return tag;
}

static char **split_command(const char *what, int *len, char **value)
{
	static char *buf;
	static char *vals[512];
	int i, what_len = strlen(what);

	bedrock_free(buf);
	buf = bedrock_strdup(what);

	*len = 0;
	*value = NULL;

	vals[(*len)++] = buf;

	for (i = 0; i < what_len; ++i)
		if (buf[i] == '.')
		{
			buf[i] = 0;
			if (i + 1 < what_len)
				vals[(*len)++] = &buf[i + 1];
		}
		else if (buf[i] == '=')
		{
			buf[i] = 0;
			if (i + 1 < what_len)
				*value = &buf[i + 1];
			break;
		}
	
	return vals;
}

static char *get_value(char *what)
{
	char *p = strchr(what, ':');
	if (p)
	{
		*p = 0;
		return p + 1;
	}

	return NULL;
}

static void process_arg(nbt_tag *tag, const char *what)
{
	int len;
	char *val;
	char **split = split_command(what, &len, &val);
	int i;

	if (!len)
		return;

	for (i = 0; i < len - 1; ++i)
	{
		char *type = split[i], *value = get_value(split[i]);
		struct tag_info *ti = tag_info_find(type);

		if (ti == NULL)
		{
			printf("Unknown tag type \"%s\"\n", type);
			return;
		}

		tag = nbt_get(tag, ti->type, 1, value);

		if (tag == NULL)
		{
			printf("No such tag \"%s\"\n", value);
			return;
		}
	}

	{
		char *type = split[len - 1], *value = get_value(split[len - 1]);
		struct tag_info *ti = tag_info_find(type);
		void *data;
		size_t sz;

		if (ti == NULL || ti->pack == NULL)
		{
			printf("Unknown tag type \"%s\"\n", type);
			return;
		}

		ti->pack(val, &data, &sz);
		nbt_set(tag, ti->type, data, sz, 1, value);
	}
}

static void write_nbt(int fd, nbt_tag *tag)
{
	bedrock_buffer *buf;
	compression_buffer *cb;
	unsigned char *p;
	int want;

	buf = nbt_write(tag);
	if (buf == NULL)
	{
		fprintf(stderr, "Unable to write NBT structure\n");
		return;
	}

	cb = compression_compress(1024, buf->data, buf->length);
	if (cb == NULL)
	{
		bedrock_buffer_free(buf);
		
		fprintf(stderr, "Unable to compress NBT structure\n");
		return;
	}

	if (lseek(fd, 0, SEEK_SET) == -1)
	{
		compression_compress_end(cb);
		bedrock_buffer_free(buf);

		fprintf(stderr, "Unable to lseek to beginning of file: %s\n", strerror(errno));
		return;
	}

	p = cb->buffer->data;
	want = cb->buffer->length;
	while (want > 0)
	{
		int i = write(fd, p, want);
		if (i <= 0)
		{
			fprintf(stderr, "Write error: %s\n", strerror(errno));

			compression_compress_end(cb);
			bedrock_buffer_free(buf);

			return;
		}

		p += i;
		want -= i;
	}

	printf("Successfully wrote %ld bytes to file\n", cb->buffer->length);

	compression_compress_end(cb);
	bedrock_buffer_free(buf);
}

int main(int argc, char **argv)
{
	nbt_tag *tag;
	int i;

	util_init();

	parse_args(argc, argv);

	fd = open(file_name, O_RDWR | _O_BINARY);
	if (fd == -1)
	{
		fprintf(stderr, "Unable to open %s: %s\n", file_name, strerror(errno));
		exit(-1);
	}

	tag = get_contents_of(fd);
	if (tag == NULL)
	{
		fprintf(stderr, "Unable to parse %s\n", file_name);
		close(fd);
		exit(-1);
	}

	for (i = 0; i < ac; ++i)
		process_arg(tag, av[i]);

	nbt_ascii_dump(stdout, tag);

	if (!pretend && ac)
		write_nbt(fd, tag);

	nbt_free(tag);
	close(fd);
	return 0;
}


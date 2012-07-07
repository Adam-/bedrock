#include "server/column.h"
#include "server/client.h"
#include "nbt/nbt.h"
#include "util/memory.h"
#include "util/endian.h"
#include "compression/compression.h"
#include "packet/packet_spawn_dropped_item.h"
#include "packet/packet_destroy_entity.h"

#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define DATA_CHUNK_SIZE 2048

struct bedrock_memory_pool column_pool = BEDROCK_MEMORY_POOL_INIT("column memory pool");
static bedrock_list dirty_columns = LIST_INIT;

struct dirty_column
{
	struct bedrock_column *column;
	bedrock_buffer *nbt_out;
	char region_path[PATH_MAX];
};

struct bedrock_column *column_create(struct bedrock_region *region, nbt_tag *data)
{
	struct bedrock_column *column = bedrock_malloc_pool(&column_pool, sizeof(struct bedrock_column));
	nbt_tag *tag;
	bedrock_node *node;
	struct nbt_tag_byte_array *byte_array;

	column->region = region;
	nbt_copy(data, TAG_INT, &column->x, sizeof(column->x), 2, "Level", "xPos");
	nbt_copy(data, TAG_INT, &column->z, sizeof(column->z), 2, "Level", "zPos");
	column->data = data;

	tag = nbt_get(data, TAG_LIST, 2, "Level", "Sections");
	LIST_FOREACH(&tag->payload.tag_list.list, node)
	{
		nbt_tag *chunk_tag = node->data;
		uint8_t y;

		nbt_copy(chunk_tag, TAG_BYTE, &y, sizeof(y), 1, "Y");
		bedrock_assert(y < sizeof(column->chunks) / sizeof(struct bedrock_chunk *), continue);

		bedrock_assert(column->chunks[y] == NULL, continue);

		chunk_load(column, y, chunk_tag);
	}
	nbt_free(tag);

	{
		compression_buffer *buffer = compression_compress_init(&column_pool, DATA_CHUNK_SIZE);

		tag = nbt_get(data, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
		byte_array = &tag->payload.tag_byte_array;
		bedrock_assert(byte_array->length == BEDROCK_BIOME_LENGTH, ;);

		compression_compress_deflate_finish(buffer, (const unsigned char *) byte_array->data, byte_array->length);
		bedrock_buffer_resize(buffer->buffer, buffer->buffer->length);

		column->biomes = buffer->buffer;
		buffer->buffer = NULL;

		nbt_free(tag);
		compression_compress_end(buffer);
	}

	return column;
}

void column_free(struct bedrock_column *column)
{
	int i;

	bedrock_assert(column->players.count == 0, ;);

	if (column->saving || column->dirty)
	{
		/* We can't free this now because it's being written to disk *or*
		 * has pending changes needing disk write.
		 * Instead detach it from the region, and it will be deleted
		 * later once the write is complete.
		 */
		column->region = NULL;
		return;
	}

	bedrock_log(LEVEL_DEBUG, "chunk: Freeing column %d,%d", column->x, column->z);

	column->items.free = (bedrock_free_func) column_free_dropped_item;
	bedrock_list_clear(&column->items);

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		chunk_free(column->chunks[i]);

	bedrock_buffer_free(column->biomes);

	nbt_free(column->data);
	bedrock_free_pool(&column_pool, column);
}

uint8_t *column_get_block(struct bedrock_column *column, int32_t x, uint8_t y, int32_t z)
{
	uint8_t chunk = y / BEDROCK_BLOCKS_PER_CHUNK;
	double column_x = (double) x / BEDROCK_BLOCKS_PER_CHUNK, column_z = (double) z / BEDROCK_BLOCKS_PER_CHUNK;
	struct bedrock_chunk *c;

	column_x = floor(column_x);
	column_z = floor(column_z);

	bedrock_assert(chunk < BEDROCK_CHUNKS_PER_COLUMN, return NULL);
	bedrock_assert(column_x == column->x && column_z == column->z, return NULL);

	c = column->chunks[chunk];

	if (c == NULL)
		return NULL;

	chunk_decompress(c);
	return chunk_get_block(c, x, y, z);
}

int32_t *column_get_height_for(struct bedrock_column *column, int32_t x, int32_t z)
{
	struct nbt_tag_int_array *tia = &nbt_get(column->data, TAG_INT_ARRAY, 2, "Level", "HeightMap")->payload.tag_int_array;
	double column_x = (double) x / BEDROCK_BLOCKS_PER_CHUNK, column_z = (double) z / BEDROCK_BLOCKS_PER_CHUNK;
	uint8_t offset;

	column_x = floor(column_x);
	column_z = floor(column_z);

	bedrock_assert(tia->length == BEDROCK_HEIGHTMAP_LENGTH, return NULL);
	bedrock_assert(column_x == column->x && column_z == column->z, return NULL);

	x %= BEDROCK_BLOCKS_PER_CHUNK;
	z %= BEDROCK_BLOCKS_PER_CHUNK;

	if (x < 0)
		x = BEDROCK_BLOCKS_PER_CHUNK - abs(x);
	if (z < 0)
		z = BEDROCK_BLOCKS_PER_CHUNK - abs(z);

	offset = z + (x * BEDROCK_BLOCKS_PER_CHUNK);

	return &tia->data[offset];
}

struct bedrock_column *find_column_which_contains(struct bedrock_region *region, double x, double z)
{
	bedrock_node *n;
	double column_x = x / BEDROCK_BLOCKS_PER_CHUNK, column_z = z / BEDROCK_BLOCKS_PER_CHUNK;
	struct bedrock_column *column = NULL;

	if (region == NULL)
		return NULL;

	column_x = floor(column_x);
	column_z = floor(column_z);

	bedrock_mutex_lock(&region->column_mutex);

	LIST_FOREACH(&region->columns, n)
	{
		struct bedrock_column *c = n->data;

		if (c->x == column_x && c->z == column_z)
		{
			column = c;
			break;
		}
		else if (c->z > column_z || (c->z == column_z && c->x > column_x))
			break;
	}

	bedrock_mutex_unlock(&region->column_mutex);

	return column;
}

void column_dirty(struct bedrock_column *column)
{
	struct dirty_column *dc;

	if (column == NULL || column->dirty == true)
		return;

	dc = bedrock_malloc(sizeof(struct dirty_column));

	dc->column = column;
	column->dirty = true;
	strncpy(dc->region_path, column->region->path, sizeof(dc->region_path));

	bedrock_list_add(&dirty_columns, dc);
}

static void column_save_entry(struct dirty_column *dc)
{
	struct bedrock_column *column = dc->column;
	int i;
	int32_t column_x, column_z;
	int offset;
	uint8_t sectors;
	uint32_t structure_start;
	compression_buffer *cb;
	int required_sectors, j;
	uint32_t header_len;
	unsigned char header[5];
	char padding[BEDROCK_REGION_SECTOR_SIZE];

	i = open(dc->region_path, O_RDWR | O_CREAT);
	if (i == -1)
	{
		bedrock_log(LEVEL_WARN, "column: Unable to open region file %s for saving - %s", dc->region_path, strerror(errno));
		return;
	}

	column_x = column->x;
	if (column_x < 0)
		column_x = BEDROCK_COLUMNS_PER_REGION - abs(column_x);

	column_z = column->z;
	if (column_z < 0)
		column_z = BEDROCK_COLUMNS_PER_REGION - abs(column_z);

	offset = column_x + column_z * BEDROCK_COLUMNS_PER_REGION;

	// Move to the offset, which contains the offset into the file of where this column starts
	if (lseek(i, offset, SEEK_SET) == -1)
	{
		bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to get offset - %s", dc->region_path, strerror(errno));
		close(i);
		return;
	}

	if (read(i, &structure_start, sizeof(structure_start)) != sizeof(structure_start))
	{
		bedrock_log(LEVEL_WARN, "column: Unable to read column structure offset in file %s to get file offset - %s", dc->region_path, strerror(errno));
		close(i);
		return;
	}

	convert_endianness((unsigned char *) &structure_start, sizeof(structure_start));

	sectors = structure_start & 0xFF;
	structure_start >>= 8;

	cb = compression_compress_init(&region_pool, DATA_CHUNK_SIZE);
	/* Compress structure */
	compression_compress_deflate_finish(cb, dc->nbt_out->data, dc->nbt_out->length);

	// Remember, this data will have a 5 byte header!
	required_sectors = (cb->buffer->length + 5) / BEDROCK_REGION_SECTOR_SIZE;
	if ((cb->buffer->length + 5) % BEDROCK_REGION_SECTOR_SIZE)
		++required_sectors;

	bedrock_log(LEVEL_DEBUG, "column: Current offset for %d,%d is %d which takes %d sectors, I need %d sectors", column->x, column->z, structure_start, sectors, required_sectors);

	if (required_sectors <= sectors)
	{
		if (lseek(i, structure_start * BEDROCK_REGION_SECTOR_SIZE, SEEK_SET) == -1)
		{
			bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to write structure - %s", dc->region_path, strerror(errno));
			compression_compress_end(cb);
			close(i);
			return;
		}

		bedrock_log(LEVEL_DEBUG, "column: Number of sectors are sufficient, moved file offset to %d", structure_start);
	}
	else
	{
		off_t pos = lseek(i, 0, SEEK_END);
		uint32_t offset_buffer;

		if (pos == -1)
		{
			bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to EOF to write structure - %s", dc->region_path, strerror(errno));
			compression_compress_end(cb);
			close(i);
			return;
		}
		else if (pos % BEDROCK_REGION_SECTOR_SIZE)
		{
			bedrock_log(LEVEL_WARN, "column: Region file %s does not have correct padding, expecting %d more bytes", dc->region_path, BEDROCK_REGION_SECTOR_SIZE - (pos % BEDROCK_REGION_SECTOR_SIZE));

			j = BEDROCK_REGION_SECTOR_SIZE - (pos % BEDROCK_REGION_SECTOR_SIZE);
			memset(&padding, 0, j);
			if (write(i, padding, j) != j)
			{
				bedrock_log(LEVEL_WARN, "column: Unable to fix padding in region file %s - %s", dc->region_path, strerror(errno));
				compression_compress_end(cb);
				close(i);
				return;
			}

			pos += j;
		}

		if (lseek(i, offset, SEEK_SET) == -1)
		{
			bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to write offset - %s", dc->region_path, strerror(errno));
			compression_compress_end(cb);
			close(i);
			return;
		}

		bedrock_log(LEVEL_DEBUG, "column: Number of sectors are NOT sufficient, moved file offset to %d to write new offset header", offset);

		bedrock_assert((pos % BEDROCK_REGION_SECTOR_SIZE) == 0, ;);
		bedrock_assert((required_sectors & ~0xFF) == 0, ;);

		offset_buffer = pos / BEDROCK_REGION_SECTOR_SIZE;
		offset_buffer <<= 8;
		offset_buffer |= required_sectors;

		convert_endianness((unsigned char *) &offset_buffer, sizeof(offset_buffer));

		if (write(i, &offset_buffer, sizeof(offset_buffer)) != sizeof(offset_buffer))
		{
			bedrock_log(LEVEL_WARN, "column: Unable to write header for new column in region file %s - %s", dc->region_path, strerror(errno));
			compression_compress_end(cb);
			close(i);
			return;
		}

		if (lseek(i, 0, SEEK_END) == -1)
		{
			bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to EOF to write structure - %s", dc->region_path, strerror(errno));
			compression_compress_end(cb);
			close(i);
			return;
		}
	}

	// Set up header
	header_len = cb->buffer->length;
	convert_endianness((unsigned char *) &header_len, sizeof(header_len));
	memcpy(&header, &header_len, sizeof(header_len));
	// Compression type
	header[4] = 2;

	if (write(i, header, sizeof(header)) != sizeof(header))
	{
		bedrock_log(LEVEL_WARN, "column: Unable to write column header for region file %s - %s", dc->region_path, strerror(errno));
		compression_compress_end(cb);
		close(i);
		return;
	}

	if (write(i, cb->buffer->data, cb->buffer->length) != cb->buffer->length)
	{
		bedrock_log(LEVEL_WARN, "column: Unable to write column structure for region file %s - %s", dc->region_path, strerror(errno));
		compression_compress_end(cb);
		close(i);
		return;
	}

	// Pad the end
	j = BEDROCK_REGION_SECTOR_SIZE - ((cb->buffer->length + 5) % BEDROCK_REGION_SECTOR_SIZE);
	bedrock_log(LEVEL_DEBUG, "column: Finished writing compressed NBT structure, file requires an additional %d bytes of padding", j);
	memset(&padding, 0, j);
	if (write(i, padding, j) != j)
	{
		bedrock_log(LEVEL_WARN, "column: Unable to write padding for region file %s - %s", dc->region_path, strerror(errno));
		compression_compress_end(cb);
		close(i);
		return;
	}

	compression_compress_end(cb);
	close(i);
}

static void column_save_exit(struct dirty_column *dc)
{
	struct bedrock_column *column = dc->column;

	column->saving = false;

	bedrock_log(LEVEL_COLUMN, "column: Finished save for column %d,%d to %s", column->x, column->z, dc->region_path);

	/* This column may need to be deleted now */
	if (column->region == NULL)
		column_free(column);

	bedrock_buffer_free(dc->nbt_out);
	bedrock_free(dc);
}

void column_save()
{
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&dirty_columns, node, node2)
	{
		struct dirty_column *dc = node->data;
		struct bedrock_column *column = dc->column;

		if (column->saving)
		{
			bedrock_log(LEVEL_WARN, "column: Column %d,%d queued for save but a save is already in progress", column->x, column->z);
			continue;
		}

		bedrock_log(LEVEL_COLUMN, "column: Starting save for column %d,%d", column->x, column->z);

		{
			int i;
			nbt_tag *level, *sections, *biomes;

			level = nbt_get(column->data, TAG_COMPOUND, 1, "Level");
			bedrock_assert(level != NULL, ;);

			sections = nbt_add(level, TAG_LIST, "Sections", NULL, 0);

			for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
			{
				struct bedrock_chunk *chunk = column->chunks[i];
				nbt_tag *chunk_tag;

				if (chunk == NULL)
					continue;

				chunk_decompress(chunk);

				chunk_tag = nbt_add(sections, TAG_COMPOUND, "", NULL, 0);

				nbt_add(chunk_tag, TAG_BYTE_ARRAY, "Data", chunk->data, BEDROCK_DATA_LENGTH);
				nbt_add(chunk_tag, TAG_BYTE_ARRAY, "SkyLight", chunk->skylight, BEDROCK_DATA_LENGTH);
				nbt_add(chunk_tag, TAG_BYTE_ARRAY, "BlockLight", chunk->blocklight, BEDROCK_DATA_LENGTH);
				nbt_add(chunk_tag, TAG_BYTE, "Y", &chunk->y, sizeof(chunk->y));
				nbt_add(chunk_tag, TAG_BYTE_ARRAY, "Blocks", chunk->blocks, BEDROCK_BLOCK_LENGTH);

				chunk_compress(chunk);
			}

			{
				compression_buffer *buf = compression_decompress(NULL, BEDROCK_BIOME_LENGTH, column->biomes->data, column->biomes->length);

				biomes = nbt_add(level, TAG_BYTE_ARRAY, "Biomes", buf->buffer->data, buf->buffer->length);

				compression_decompress_end(buf);
			}

			nbt_ascii_dump(column->data);
			dc->nbt_out = nbt_write(column->data);

			nbt_free(sections);
			nbt_free(biomes);
		}

		column->dirty = false;
		column->saving = true;

		bedrock_thread_start((bedrock_thread_entry) column_save_entry, (bedrock_thread_exit) column_save_exit, dc);

		bedrock_list_del_node(&dirty_columns, node);
		bedrock_free(node);
	}
}

void column_add_item(struct bedrock_column *column, struct bedrock_dropped_item *di)
{
	bedrock_node *node;

	bedrock_log(LEVEL_DEBUG, "column: Creating dropped item %s in at %f,%f,%f", di->item->name, di->x, di->y, di->z);

	bedrock_assert(di->eid == 0, ;);
	di->eid = ++entity_id;
	di->column = column;

	bedrock_list_add(&column->items, di);

	/* Send out this item to nearby players */
	LIST_FOREACH(&column->players, node)
	{
		struct bedrock_client *client = node->data;

		packet_send_spawn_dropped_item(client, di);
	}
}

void column_free_dropped_item(struct bedrock_dropped_item *item)
{
	bedrock_node *node;

	bedrock_log(LEVEL_DEBUG, "column: Freeing dropped item %s at %f,%f,%f", item->item->name, item->x, item->y, item->z);

	/* Delete this item from nearby players */
	LIST_FOREACH(&item->column->players, node)
	{
		struct bedrock_client *client = node->data;

		packet_send_destroy_entity_dropped_item(client, item);
	}

	bedrock_list_del(&item->column->items, item);
	bedrock_free(item);
}

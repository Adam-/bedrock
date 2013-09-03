#include "server/column.h"
#include "server/client.h"
#include "blocks/items.h"
#include "blocks/blocks.h"
#include "nbt/nbt.h"

enum
{
	ENTITY_CHEST_SLOTS = 27,
	ENTITY_FURNACE_SLOTS = 3
};

struct tile_entity
{
	int blockid; /* Item id */
	int x;       /* Item coords */
	int y;
	int z;

	void (*on_free)(struct tile_entity *);
	struct column *column; /* column this entity is in */
	bedrock_list clients; /* clients looking at this entity */
};

struct chest
{
	/* id = BLOCK_CHEST */
	struct tile_entity entity;
	struct item_stack items[ENTITY_CHEST_SLOTS];
};

struct furnace
{
	struct tile_entity entity;
	struct item_stack in, fuel, out;

	bool burning; /* whether or not this furnace is currently lit */
	int16_t progress_arrow; /* this goes from 0 to 200 every 10 seconds */
	int16_t fuel_indicator; /* this goes from 0 to 200 every X/200 ticks, where X = length of time fuel lasts in ticks */
};

extern void entity_load(struct column *column, nbt_tag *data);
extern void entity_save(struct column *column);
extern void entity_cleanup(struct column *column);
extern void entity_free(struct tile_entity *entity);
extern void entity_operate(struct client *client, struct tile_entity *entity);

extern struct tile_entity *chest_load(nbt_tag *tag);
extern void chest_save(nbt_tag *tag, struct tile_entity *entity);
extern void chest_operate(struct client *client, struct tile_entity *entity);
extern void chest_mine(struct client *, struct chunk *, int32_t x, uint8_t y, int32_t z, struct block *block, bool can_harvest);
extern void chest_place(struct client *client, struct chunk *chunk, int32_t x, uint8_t y, int32_t z, struct block *block);
extern void chest_propagate(struct chest *chest);

extern struct tile_entity *furnace_load(nbt_tag *tag);
extern void furnace_save(nbt_tag *tag, struct tile_entity *entity);
extern void furnace_operate(struct client *client, struct tile_entity *entity);
extern void furnace_propagate(struct furnace *furnace);
extern void furnace_burn(struct furnace *furnace);
extern void furnace_tick(uint64_t diff);


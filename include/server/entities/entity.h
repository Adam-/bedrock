#include "server/column.h"
#include "server/client.h"
#include "blocks/items.h"
#include "nbt/nbt.h"

#define ENTITY_CHEST_SLOTS 27

struct tile_entity
{
	int blockid; /* Item id */
	int x;       /* Item coords */
	int y;
	int z;
};

struct chest
{
	/* id = BLOCK_CHEST */
	struct tile_entity entity;
	struct item_stack items[ENTITY_CHEST_SLOTS];
};


extern void entity_load(struct column *column, nbt_tag *data);
extern void entity_save(struct column *column);
extern void entity_cleanup(struct column *column);
extern void entity_free(struct tile_entity *entity);
extern void entity_operate(struct client *client, struct tile_entity *entity);

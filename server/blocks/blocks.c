#include "server/bedrock.h"
#include "blocks/blocks.h"
#include "server/column.h"
#include "entities/entity.h"

void simple_drop(struct client bedrock_attribute_unused *client, struct chunk *chunk, int32_t x, uint8_t y, int32_t z, struct block *block, bool harvest)
{
	if (!harvest)
		return;

	struct dropped_item *di = bedrock_malloc(sizeof(struct dropped_item));
	di->item = item_find_or_create(block->id);
	di->count = 1;
	di->data = 0;
	di->x = x;
	di->y = y;
	di->z = z;

	column_add_item(chunk->column, di);
}

struct block blocks[] = {
	{BLOCK_AIR,                   "Air",                -1,    -1,    ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_STONE,                 "Stone",               2.25,  7.5,  ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_GRASS,                 "Grass",               0.9,   0.9,  ITEM_FLAG_SHOVEL,                           ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_DIRT,                  "Dirt",                0.75,  0.75, ITEM_FLAG_SHOVEL,                           ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_COBBLESTONE,           "Cobblestone",         3,     9,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_WOODEN_PLANKS,         "Wooden Planks",       3,     3,    ITEM_FLAG_AXE,                              ITEM_FLAG_AXE,                              simple_drop, NULL},
	{BLOCK_SAPLINGS,              "Saplings",            0.05,  0.05, ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_BEDROCK,               "Bedrock",            -1,    -1,    ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_WATER,                 "Water",              -1,    -1,    ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_STATIONARY_WATER,      "Water",              -1,    -1,    ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_LAVA,                  "Lava",               -1,    -1,    ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_STATIONARY_LAVA,       "Lava",               -1,    -1,    ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_SAND,                  "Sand",                0.75,  0.75, ITEM_FLAG_SHOVEL,                           ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_GRAVEL,                "Gravel",              0.9,   0.9,  ITEM_FLAG_SHOVEL,                           ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_GOLD_ORE,              "Gold Ore",            4.5,   15,   ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_STONE,   ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    simple_drop, NULL},
	{BLOCK_IRON_ORE,              "Iron Ore",            4.5,   15,   ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_STONE,   ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_STONE,   simple_drop, NULL},
	{BLOCK_COAL_ORE,              "Coal Ore",            4.5,   15,   ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_WOOD,                  "Wood",                3,     3,    ITEM_FLAG_AXE,                              ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_LEAVES,                "Leaves",              0.3,  0.3,   ITEM_FLAG_NONE,                             ITEM_FLAG_SHEARS,                           simple_drop, NULL},
	{BLOCK_SPONGE,                "Sponge",              0.9,  0.9,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_GLASS,                 "Glass",               0.5,  0.5,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_LAPIS_LAZULI_ORE,      "Lapis Lazuli Ore",    4.5,  15,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_STONE,   ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_STONE,   simple_drop, NULL},
	{BLOCK_LAPIS_LAZULI_BLOCK ,   "Lapis Lazuli Block",  4.5,  15,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_STONE,   ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_STONE,   simple_drop, NULL},
	{BLOCK_DISPENSER,             "Dispenser",           5.25, 17.5,  ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_SANDSTONE,             "Sandstone",           1.2,  4,     ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_NOTE_BLOCK,            "Note Block",          1.2,  1.2,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_BED,                   "Bed",                 0.3,  0.3,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_POWERED_RAIL,          "Powered Rail",        1.05, 1.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_DETECTOR_RAIL,         "Detector Rail",       1.05, 1.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_STICKY_PISTON,         "Sticky Piston",       0.75, 0.75,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_WOOL,                  "Wool",                1.2,  1.2,   ITEM_FLAG_NONE,                             ITEM_FLAG_SHEARS,                           simple_drop, NULL},
	{BLOCK_DANDELION,             "Dandelion",           0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_ROSE,                  "Rose",                0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_BROWN_MUSHROOM,        "Mushroom",            0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_RED_MUSHROOM,          "Mushroom",            0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_OF_GOLD,               "Block of Gold",       4.5,  15,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    simple_drop, NULL},
	{BLOCK_OF_IRON,               "Block of Iron",       7.5,  25,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    simple_drop, NULL},
	{BLOCK_DOUBLE_SLABS,          "Slabs",               3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_SLABS,                 "Block Slabs",         3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_BRICKS,                "Bricks",              3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_TNT,                   "TNT",                 0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_BOOKSHELF,             "Bookshelf",           2.25, 2.25,  ITEM_FLAG_AXE,                              ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_MOSS_STONE,            "Moss Stone",          3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_OBSIDIAN,              "Obsidian",            10,   250,   ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_DIAMOND, ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_DIAMOND, simple_drop, NULL},
	{BLOCK_TORCH,                 "Torch",               0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_FIRE,                  "Fire",                0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_MONSTER_SPAWNER,       "Monster Spawner",     7.5,  25,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_WOODEN_STAIRS,         "Wooden Stairs",       3,    10,    ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_CHEST,                 "Chest",               3.75, 3.75,  ITEM_FLAG_AXE,                              ITEM_FLAG_AXE,                              chest_mine, chest_place},
	{BLOCK_REDSTONE_WIRE,         "Redstone",            0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_DIAMOND_ORE,           "Diamond Ore",         7.5,  15,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    simple_drop, NULL},
	{BLOCK_DIAMOND,               "Block of Diamond",    7.5,  25,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    simple_drop, NULL},
	{BLOCK_CRAFTING_TABLE,        "Crafting Table",      3.75, 3.75,  ITEM_FLAG_AXE,                              ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_WHEAT_SEEDS,           "Wheat Seeds",         0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_FARMLAND,              "Farmland",            0.9,  0.9,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_FURNACE,               "Furnace",             5.25, 17.5,  ITEM_FLAG_PICKAXE,                          ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_BURNING_FURNACE,       "Furnace",             5.25, 5.25,  ITEM_FLAG_PICKAXE,                          ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_SIGN_POST,             "Sign Post",           1.5,  1.5,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_WOODEN_DOOR,           "Wood Door",           4.5,  4.5,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_LADDERS,               "Ladder",              0.6,  0.6,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_RAILS,                 "Rails",               1.05, 1.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_COBBLESTONE_STAIRS,    "Cobblestone Stairs",  3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_WALL_SIGN,             "Wall Sign",           1.5,  1.5,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_LEVER,                 "Lever",               0.75, 0.75,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_STONE_PRESSURE_PLATE,  "Pressure Plate",      0.75, 0.75,  ITEM_FLAG_PICKAXE,                          ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_IRON_DOOR,             "Iron Door",           7.5,  25,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_WOODEN_PRESSURE_PLATE, "Pressure Plate",      0.75, 0.75,  ITEM_FLAG_PICKAXE,                          ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_REDSTONE_ORE,          "Redstone Ore",        4.5,  15,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    simple_drop, NULL},
	{BLOCK_GLOWING_REDSTONE_ORE,  "Redstone Ore",        4.5,  15,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    ITEM_FLAG_PICKAXE | TOOL_TYPE_MASK_IRON,    simple_drop, NULL},
	{BLOCK_REDSTONE_TORCH_OFF,    "Redstone Torch",      0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_REDSTONE_TORCH_ON,     "Redstone Torch",      0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_STONE_BUTTON,          "Button",              0.75, 0.75,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_SNOW,                  "Snow",                0.15, 0.5,   ITEM_FLAG_SHOVEL,                           ITEM_FLAG_SHOVEL,                           simple_drop, NULL},
	{BLOCK_ICE,                   "Ice",                 0.75, 2.5,   ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_SNOW_BLOCK,            "Snow Block",          0.3,  1,     ITEM_FLAG_SHOVEL,                           ITEM_FLAG_SHOVEL,                           simple_drop, NULL},
	{BLOCK_CACTUS,                "Cactus",              0.6,  0.6,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_CLAY_BLOCK,            "Clay",                0.9,  0.9,   ITEM_FLAG_SHOVEL,                           ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_SUGAR_CANE,            "Reed",                0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_JUKEBOX,               "Jukebox",             3,    3,     ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_FENCE,                 "Fence",               3,    3,     ITEM_FLAG_AXE,                              ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_PUMPKIN,               "Pumpkin",             1.5,  1.5,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_NETHERRACK,            "Netherrack",          0.6,  2,     ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_SOUL_SAND,             "Soul Sand",           0.75, 0.75,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_GLOWSTONE_BLOCK,       "Glowstone",           0.45, 0.45,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_PORTAL,                "Portal",             -1,   -1,     ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_JACK_O_LANTERN,        "Jack O Lantern",      1.5,  1.5,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_CAKE,                  "Cake",                0.75, 0.75,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_REDSTONE_REPEATER_OFF, "Redstone Repeater",   0.5,  0.5,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_REDSTONE_REPEATER_ON,  "Redstone Repeater",   0.5,  0.5,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_LOCKED_CHEST,          "Locked Chest",        3.75, 3.75,  ITEM_FLAG_AXE,                              ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_TRAPDOOR,              "Trapdoor",            4.5,  4.5,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_MONSTER_EGG,           "Monster Egg",        -1,   -1,     ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_STONE_BRICKS,          "Stone Bricks",        2.25, 7.5,   ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_HUGE_BROWN_MUSHROOM,   "Huge Brown Mushroom", 0.3,  0.3,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_HUGE_RED_MUSHROOM,     "Huge Red Mushroom",   0.3,  0.3,   ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_IRON_BARS,             "Iron Bars",           7.5,  25,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_GLASS_PANE,            "Glass Pane",          0.45, 0.45,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_MELON,                 "Melon",               0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_PUMPKIN_STEM,          "Pumpkin Stem",        3.75, 3.75,  ITEM_FLAG_AXE,                              ITEM_FLAG_AXE,                              simple_drop, NULL},
	{BLOCK_MELON_STEM,            "Melon Stem",          3.75, 3.75,  ITEM_FLAG_AXE,                              ITEM_FLAG_AXE,                              simple_drop, NULL},
	{BLOCK_VINES,                 "Vines",               0.3,  0.3,   ITEM_FLAG_NONE,                             ITEM_FLAG_SHEARS,                           simple_drop, NULL},
	{BLOCK_FENCED_GATE,           "Fenced Gate",         3,    3,     ITEM_FLAG_AXE,                              ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_BRICK_STAIRS,          "Brick Stairs",        3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_STONE_BRICK_STAIRS,    "Stone Brick Stairs",  3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_MYCELIUM,              "Mycelium",            0.9,  0.9,   ITEM_FLAG_SHOVEL,                           ITEM_FLAG_SHOVEL,                           simple_drop, NULL},
	{BLOCK_LILY_PAD,              "Lily Pad",            0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_SHEARS,                           simple_drop, NULL},
	{BLOCK_NETHER_BRICK,          "Nether Brick",        3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_NETHER_BRICK_FENCE,    "Nether Brick Fence",  3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_NETHER_BRICK_STAIRS,   "Nether Brick Stairs", 3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_NETHER_WART,           "Nether Wart",         0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_ENCHANTMENT_TABLE,     "Enchantment Table",   7.5,  25,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_BREWING_STAND,         "Brewing Stand",       0.75, 0.75,  ITEM_FLAG_AXE,                              ITEM_FLAG_AXE,                              simple_drop, NULL},
	{BLOCK_CAULDRON,              "Cauldron",            3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_END_PORTAL,            "End Portal",         -1,   -1,     ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_END_PORTAL_FRAME,      "End Portal Frame",    4.5,  15,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_END_STONE,             "End Stone",           4.5,  15,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_DRAGON_EGG,            "Dragon Egg",         -1,   -1,     ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             NULL, NULL},
	{BLOCK_REDSTONE_LAMP_OFF,     "Redstone Lamp",       0.45, 0.45,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_REDSTONE_LAMP_ON,      "Redstone Lamp",       3.75, 3.75,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_WOODEN_DOUBLE_SLAB,    "Wood Slab",           3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_COCOA_PLANT,           "Cocoa Plant",         0.05, 0.05,  ITEM_FLAG_NONE,                             ITEM_FLAG_NONE,                             simple_drop, NULL},
	{BLOCK_SANDSTONE_STAIRS,      "Sandstone Stairs",    3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_EMERALD_ORE,           "Emerald Ore",         3,    10,    ITEM_FLAG_PICKAXE,                          ITEM_FLAG_PICKAXE,                          simple_drop, NULL},
	{BLOCK_ENDER_CHEST,           "Ender Chest",         3.75, 3.75,  ITEM_FLAG_AXE,                              ITEM_FLAG_AXE,                              simple_drop, NULL}
};

static int block_compare(const enum block_type *id, const struct block *block)
{
	if (*id < block->id)
		return -1;
	else if (*id > block->id)
		return 1;
	return 0;
}

typedef int (*compare_func)(const void *, const void *);

struct block *block_find(enum block_type id)
{
	return bsearch(&id, blocks, sizeof(blocks) / sizeof(struct block), sizeof(struct block), (compare_func) block_compare);
}

struct block *block_find_or_create(enum block_type id)
{
	static struct block b;
	struct block *block = block_find(id);

	if (block == NULL)
	{
		bedrock_log(LEVEL_DEBUG, "block: Unrecognized block %d", id);

		b.id = id;
		b.name = "Unknown";
		b.hardness = 0;
		b.no_harvest_time = 0;
		b.weakness = ITEM_FLAG_NONE;
		b.harvest = ITEM_FLAG_NONE;
		b.on_mine = NULL;
		b.on_place = NULL;

		block = &b;
	}

	return block;
}

struct block *block_find_by_name(const char *name)
{
	unsigned i;

	for (i = 0; i < sizeof(blocks) / sizeof(struct block); ++i)
	{
		struct block *block = &blocks[i];

		if (!strcmp(name, block->name))
			return block;
	}

	return NULL;
}


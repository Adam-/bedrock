#include "blocks/items.h"

#include <stdint.h>

typedef enum
{
	BLOCK_AIR,                   /* 0 */
	BLOCK_STONE,                 /* 1 */
	BLOCK_GRASS,                 /* 2 */
	BLOCK_DIRT,                  /* 3 */
	BLOCK_COBBLESTONE,           /* 4 */
	BLOCK_WOODEN_PLANKS,         /* 5 */
	BLOCK_SAPLINGS,              /* 6 */
	BLOCK_BEDROCK,               /* 7 */
	BLOCK_WATER,                 /* 8 */
	BLOCK_STATIONARY_WATER,      /* 9 */
	BLOCK_LAVA,                  /* 10 */
	BLOCK_STSATIONARY_LAVA,      /* 11 */
	BLOCK_SAND,                  /* 12 */
	BLOCK_GRAVEL,                /* 13 */
	BLOCK_GOLD_ORE,              /* 14 */
	BLOCK_IRON_ORE,              /* 15 */
	BLOCK_COAL_ORE,              /* 16 */
	BLOCK_WOOD,                  /* 17 */
	BLOCK_LEAVES,                /* 18 */
	BLOCK_SPONGE,                /* 19 */
	BLOCK_GLASS,                 /* 20 */
	BLOCK_LAPIS_LAZULI_ORE,      /* 21 */
	BLOCK_LAPIS_LAZULI_BLOCK,    /* 22 */
	BLOCK_DISPENSER,             /* 23 */
	BLOCK_SANDSTONE,             /* 24 */
	BLOCK_NOTE_BLOCK,            /* 25 */
	BLOCK_BED,                   /* 26 */
	BLOCK_POWERED_RAIL,          /* 27 */
	BLOCK_DETECTOR_RAIL,         /* 28 */
	BLOCK_STICKY_PISTON,         /* 29 */
	BLOCK_COBWEB,                /* 30 */
	BLOCK_TALL_GRASS,            /* 31 */
	BLOCK_DEAD_BUSH,             /* 32 */
	BLOCK_PISTON,                /* 33 */
	BLOCK_PISTON_EXTENSION,      /* 34 */
	BLOCK_WOOL,                  /* 35 */
	BLOCK_MOVED_BY_PISTON,       /* 36 */
	BLOCK_DANDELION,             /* 37 */
	BLOCK_ROSE,                  /* 38 */
	BLOCK_BROWN_MUSHROOM,        /* 39 */
	BLOCK_RED_MUSHROOM,          /* 40 */
	BLOCK_OF_GOLD,               /* 41 */
	BLOCK_OF_IRON,               /* 42 */
	BLOCK_DOUBLE_SLABS,          /* 43 */
	BLOCK_SLABS,                 /* 44 */
	BLOCK_BRICKS,                /* 45 */
	BLOCK_TNT,                   /* 46 */
	BLOCK_BOOKSHELF,             /* 47 */
	BLOCK_MOSS_STONE,            /* 48 */
	BLOCK_OBSIDIAN,              /* 49 */
	BLOCK_TORCH,                 /* 50 */
	BLOCK_FIRE,                  /* 51 */
	BLOCK_MONSTER_SPAWNER,       /* 52 */
	BLOCK_WOODEN_STAIRS,         /* 53 */
	BLOCK_CHEST,                 /* 54 */
	BLOCK_REDSTONE_WIRE,         /* 55 */
	BLOCK_DIAMOND_ORE,           /* 56 */
	BLOCK_DIAMOND,               /* 57 */
	BLOCK_CRAFTING_TABLE,        /* 58 */
	BLOCK_WHEAT_SEEDS,           /* 59 */
	BLOCK_FARMLAND,              /* 60 */
	BLOCK_FURNACE,               /* 61 */
	BLOCK_BURNING_FURNACE,       /* 62 */
	BLOCK_SIGN_POST,             /* 63 */
	BLOCK_WOODEN_DOOR,           /* 64 */
	BLOCK_LADDERS,               /* 65 */
	BLOCK_RAILS,                 /* 66 */
	BLOCK_COBBLESTONE_STAIRS,    /* 67 */
	BLOCK_WALL_SIGN,             /* 68 */
	BLOCK_LEVER,                 /* 69 */
	BLOCK_STONE_PRESSURE_PLATE,  /* 70 */
	BLOCK_IRON_DOOR,             /* 71 */
	BLOCK_WOODEN_PRESSURE_PLATE, /* 72 */
	BLOCK_REDSTONE_ORE,          /* 73 */
	BLOCK_GLOWING_REDSTONE_ORE,  /* 74 */
	BLOCK_REDSTONE_TORCH_OFF,    /* 75 */
	BLOCK_REDSTONE_TORCH_ON,     /* 76 */
	BLOCK_STONE_BUTTON,          /* 77 */
	BLOCK_SNOW,                  /* 78 */
	BLOCK_ICE,                   /* 79 */
	BLOCK_SNOW_BLOCK,            /* 80 */
	BLOCK_CACTUS,                /* 81 */
	BLOCK_CLAY_BLOCK,            /* 82 */
	BLOCK_SUGAR_CANE,            /* 83 */
	BLOCK_JUKEBOX,               /* 84 */
	BLOCK_FENCE,                 /* 85 */
	BLOCK_PUMPKIN,               /* 86 */
	BLOCK_NETHERRACK,            /* 87 */
	BLOCK_SOUL_SAND,             /* 88 */
	BLOCK_GLOWSTONE_BLOCK,       /* 89 */
	BLOCK_PORTAL,                /* 90 */
	BLOCK_JACK_O_LANTERN,        /* 91 */
	BLOCK_CAKE_BLOCK,            /* 92 */
	BLOCK_REDSTONE_REPEATER_OFF, /* 93 */
	BLOCK_REDSTONE_REPEATER_ON,  /* 94 */
	BLOCK_LOCKED_CHEST,          /* 95 */
	BLOCK_TRAPDOOR,              /* 96 */
	BLOCK_MONSTER_EGG,           /* 97 */
	BLOCK_STONE_BRICKS,          /* 98 */
	BLOCK_HUGE_BROWN_MUSHROOM,   /* 99 */
	BLOCK_HUGE_RED_MUSHROOM,     /* 100 */
	BLOCK_IRON_BARS,             /* 101 */
	BLOCK_GLASS_PANE,            /* 102 */
	BLOCK_MELON,                 /* 103 */
	BLOCK_PUMPKIN_STEM,          /* 104 */
	BLOCK_MELON_STEM,            /* 105 */
	BLOCK_VINES,                 /* 106 */
	BLOCK_FENCED_GATE,           /* 107 */
	BLOCK_BRICK_STAIRS,          /* 108 */
	BLOCK_STONE_BRICK_STAIRS,    /* 109 */
	BLOCK_MYCELIUM,              /* 110 */
	BLOCK_LILY_PAD,              /* 111 */
	BLOCK_NETHER_BRICK,          /* 112 */
	BLOCK_NETHER_BRICK_FENCE,    /* 113 */
	BLOCK_NETHER_BRICK_STAIRS,   /* 114 */
	BLOCK_NETHER_WART,           /* 115 */
	BLOCK_ENCHANTMENT_TABLE,     /* 116 */
	BLOCK_BREWING_STAND,         /* 117 */
	BLOCK_CAULDRON,              /* 118 */
	BLOCK_END_PORTAL,            /* 119 */
	BLOCK_END_PORTAL_FRAME,      /* 120 */
	BLOCK_END_STONE,             /* 121 */
	BLOCK_DRAGON_EGG,            /* 122 */
	BLOCK_REDSTONE_LAMP_OFF,     /* 123 */
	BLOCK_REDSTONE_LAMP_ON,      /* 124 */
	BLOCK_WOODEN_DOUBLE_SLAB,    /* 125 */
	BLOCK_WOODEN_SLAB,           /* 126 */
	BLOCK_COCOA_PLANT,           /* 127 */
	BLOCK_SANDSTONE_STAIRS,      /* 128 */
	BLOCK_EMERALD_ORE,           /* 129 */
	BLOCK_ENDER_CHEST            /* 130 */
} block_type;

struct bedrock_block
{
	uint8_t id;
	const char *name;
	double hardness;                     /* Hardness. Time it takes to mine this block with different tools is calculated by this. */
	double no_harvest_time;              /* Time in seconds it takes to mine this block without the required tools to harvest the block. */
	enum bedrock_item_flags weakness;    /* Types of tools (axe/pickaxe/hatchet) that speed up mining this block */
	enum bedrock_item_flags requirement; /* Types of tools that are required to mine this block */
	enum bedrock_item_flags harvest;     /* Types of tools (material) required to get resources from this block */
	void (*on_mine)(struct bedrock_client *, struct bedrock_block *); /* Called when a block is successfully mined (with the correct tools). Should probably spawn resources. */
};

extern struct bedrock_block bedrock_blocks[];

extern struct bedrock_block *block_find(block_type id);
extern struct bedrock_block *block_find_or_create(block_type id);

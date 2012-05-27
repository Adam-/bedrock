#include <stdint.h>

typedef enum
{
	ITEM_IRON_SHOVEL = 256,      /* 256 */
	ITEM_IRON_PICKAXE,           /* 257 */
	ITEM_IRON_AXE,               /* 258 */
	ITEM_FLINT_AND_STEEL,        /* 259 */
	ITEM_RED_APPLE,              /* 260 */
	ITEM_BOW,                    /* 261 */
	ITEM_ARROW,                  /* 262 */
	ITEM_COAL,                   /* 263 */
	ITEM_DIAMOND,                /* 264 */
	ITEM_IRON_INGOT,             /* 265 */
	ITEM_GOLD_INGOT,             /* 266 */
	ITEM_IRON_SWORD,             /* 267 */
	ITEM_WOODEN_SWORD,           /* 268 */
	ITEM_WOODEN_SHOVEL,          /* 269 */
	ITEM_WOODEN_PICKAXE,         /* 270 */
	ITEM_WOODEN_AXE,             /* 271 */
	ITEM_STONE_SWORD,            /* 272 */
	ITEM_STONE_SHOVEL,           /* 273 */
	ITEM_STONE_PICKAXE,          /* 274 */
	ITEM_STONE_AXE,              /* 275 */
	ITEM_DIAMOND_SWORD,          /* 276 */
	ITEM_DIAMOND_SHOVEL,         /* 277 */
	ITEM_DIAMOND_PICKAXE,        /* 278 */
	ITEM_DIAMOND_AXE,            /* 279 */
	ITEM_STICK,                  /* 280 */
	ITEM_BOWL,                   /* 281 */
	ITEM_MUSHROOM_SOUP,          /* 282 */
	ITEM_GOLD_SWORD,             /* 283 */
	ITEM_GOLD_SHOVEL,            /* 284 */
	ITEM_GOLD_PICKAXE,           /* 285 */
	ITEM_GOLD_AXE,               /* 286 */
	ITEM_STRING,                 /* 287 */
	ITEM_FEATHER,                /* 288 */
	ITEM_GUNPOWDER,              /* 289 */
	ITEM_WOODEN_HOE,             /* 290 */
	ITEM_STONE_HOE,              /* 291 */
	ITEM_IRON_HOE,               /* 292 */
	ITEM_DIAMOND_HOE,            /* 293 */
	ITEM_GOLD_HOE,               /* 294 */
	ITEM_SEEDS,                  /* 295 */
	ITEM_WHEAT,                  /* 296 */
	ITEM_BREAD,                  /* 297 */
	ITEM_LEATHER_CAP,            /* 298 */
	ITEM_LEATHER_TUNIC,          /* 299 */
	ITEM_LEATHER_PANTS,          /* 300 */
	ITEM_LEATHER_BOOTS,          /* 301 */
	ITEM_CHAIN_HELMET,           /* 302 */
	ITEM_CHAIN_CHESTPLATE,       /* 303 */
	ITEM_CHAIN_LEGGINGS,         /* 304 */
	ITEM_CHAIN_BOOTS,            /* 305 */
	ITEM_IRON_HELMET,            /* 306 */
	ITEM_IRON_CHESTPLATE,        /* 307 */
	ITEM_IRON_LEGGINGS,          /* 308 */
	ITEM_IRON_BOOTS,             /* 309 */
	ITEM_DIAMOND_HELMET,         /* 310 */
	ITEM_DIAMOND_CHESTPLATE,     /* 311 */
	ITEM_DIAMOND_LEGGINGS,       /* 312 */
	ITEM_DIAMOND_BOOTS,          /* 313 */
	ITEM_GOLDEN_HELMET,          /* 314 */
	ITEM_GOLDEN_CHESTPLATE,      /* 315 */
	ITEM_GOLDEN_LEGGINGS,        /* 316 */
	ITEM_GOLDEN_BOOTS,           /* 317 */
	ITEM_FLINT,                  /* 318 */
	ITEM_RAW_PORKCHOP,           /* 319 */
	ITEM_COOKED_PORKCHOP,        /* 320 */
	ITEM_PAINTINGS,              /* 321 */
	ITEM_GOLDEN_APPLE,           /* 322 */
	ITEM_SIGN,                   /* 323 */
	ITEM_WOODEN_DOOR,            /* 324 */
	ITEM_BUCKET,                 /* 325 */
	ITEM_WATER_BUCKET,           /* 326 */
	ITEM_LAVA_BUCKET,            /* 327 */
	ITEM_MINECART,               /* 328 */
	ITEM_SADDLE,                 /* 329 */
	ITEM_IRON_DOOR,              /* 330 */
	ITEM_REDSTONE_DUST,          /* 331 */
	ITEM_SNOWBALL,               /* 332 */
	ITEM_BOAT,                   /* 333 */
	ITEM_LEATHER,                /* 334 */
	ITEM_MILK_BUCKET,            /* 335 */
	ITEM_CLAY_BRICK,             /* 336 */
	ITEM_CLAY,                   /* 337 */
	ITEM_SUGAR_CANE,             /* 338 */
	ITEM_PAPER,                  /* 339 */
	ITEM_BOOK,                   /* 340 */
	ITEM_SLIMEBALL,              /* 341 */
	ITEM_MINECRAFT_WITH_CHEST,   /* 342 */
	ITEM_MINECRAFT_WITH_FURNACE, /* 343 */
	ITEM_CHICKEN_EGG,            /* 344 */
	ITEM_COMPASS,                /* 345 */
	ITEM_FISHING_ROD,            /* 346 */
	ITEM_CLOCK,                  /* 347 */
	ITEM_GLOWSTONE_DUST,         /* 348 */
	ITEM_RAW_FISH,               /* 349 */
	ITEM_COOKED_FISH,            /* 350 */
	ITEM_DYE,                    /* 351 */
	ITEM_BONE,                   /* 352 */
	ITEM_SUGAR,                  /* 353 */
	ITEM_CAKE,                   /* 354 */
	ITEM_BED,                    /* 355 */
	ITEM_REDSTONE_REPEATER,      /* 356 */
	ITEM_COOKIE,                 /* 357 */
	ITEM_MAP,                    /* 358 */
	ITEM_SHEARS,                 /* 359 */
	ITEM_MELON_SLICE,            /* 360 */
	ITEM_PUMPKIN_SEEDS,          /* 361 */
	ITEM_MELON_SEEDS,            /* 362 */
	ITEM_RAW_BEEF,               /* 363 */
	ITEM_STEAK,                  /* 364 */
	ITEM_RAW_CHICKEN,            /* 365 */
	ITEM_COOKED_CHICKEN,         /* 366 */
	ITEM_ROTTEN_FLESH,           /* 367 */
	ITEM_ENDER_PEARL,            /* 368 */
	ITEM_BLAZE_ROD,              /* 369 */
	ITEM_GHAST_TEAR,             /* 370 */
	ITEM_GOLD_NUGGET,            /* 371 */
	ITEM_NETHER_WART,            /* 372 */
	ITEM_POTIONS,                /* 373 */
	ITEM_GLASS_BOTTLE,           /* 374 */
	ITEM_SPIDER_EYE,             /* 375 */
	ITEM_FERMENTED_SPIDER_EYE,   /* 376 */
	ITEM_BLAZE_POWDER,           /* 377 */
	ITEM_MAGMA_CREAM,            /* 378 */
	ITEM_BREWING_STAND,          /* 379 */
	ITEM_CAULDRON,               /* 380 */
	ITEM_EYE_OF_ENDER,           /* 381 */
	ITEM_GLISTERING_MELON,       /* 382 */
	ITEM_SPAWN_EGG,              /* 383 */
	ITEM_BOTTLE_OF_ENCHANTING,   /* 384 */
	ITEM_FIRE_CHARGE,            /* 385 */
	ITEM_BOOK_AND_QUILL,         /* 386 */
	ITEM_WRITTEN_BOOK,           /* 387 */
	ITEM_EMERALD,                /* 388 */
	ITEM_13_DISC = 2256,         /* 2256 */
	ITEM_CAT_DISC,               /* 2257 */
	ITEM_BLOCKS_DISC,            /* 2258 */
	ITEM_CHIRP_DISC,             /* 2259 */
	ITEM_FAR_DISC,               /* 2260 */
	ITEM_MALL_DISC,              /* 2261 */
	ITEM_MELLOHI_DISC,           /* 2262 */
	ITEM_STAL_DISC,              /* 2263 */
	ITEM_STRAD_DISC,             /* 2264 */
	ITEM_WARD_DISC,              /* 2265 */
	ITEM_11_DISC                 /* 2266 */
} item_type;

enum
{
	ITEM_FLAG_NONE,
	ITEM_FLAG_DAMAGABLE = 1 << 0,
	ITEM_FLAG_ENCHANTABLE = 1 << 1
};

struct bedrock_item
{
	uint16_t id;
	const char *name;
	uint8_t flags;
};

extern struct bedrock_item bedrock_items[];

extern struct bedrock_item *item_find(item_type id);
extern struct bedrock_item *item_find_or_create(item_type id);

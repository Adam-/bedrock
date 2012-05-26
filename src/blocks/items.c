#include "server/bedrock.h"
#include "blocks/items.h"

struct bedrock_item bedrock_items[] = {
	{ITEM_FLINT_AND_STEEL,    "Flint and steel",    ITEM_FLAG_DAMAGABLE},
	{ITEM_BOW,                "Bow",                ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_WOODEN_SWORD,       "Wooden sword",       ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_WOODEN_SHOVEL,      "Wooden shovel",      ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_WOODEN_PICKAXE,     "Wooden pickaxe",     ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_WOODEN_AXE,         "Wooden axe",         ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_STONE_SWORD,        "Stone sword",        ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_STONE_SHOVEL,       "Stone shovel",       ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_STONE_PICKAXE,      "Stone pickaxe",      ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_STONE_AXE,          "Stone axe",          ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_DIAMOND_SWORD,      "Diamond sword",      ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_DIAMOND_SHOVEL,     "Diamond shovel",     ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_DIAMOND_PICKAXE,    "Diamond pickaxe",    ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_DIAMOND_AXE,        "Diamond axe",        ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_GOLD_SWORD,         "Gold sword",         ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_GOLD_SHOVEL,        "Gold shovel",        ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_GOLD_PICKAXE,       "Gold pickaxe",       ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_GOLD_AXE,           "Gold axe",           ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_WOODEN_HOE,         "Wooden hoe",         ITEM_FLAG_DAMAGABLE},
	{ITEM_STONE_HOE,          "Stone hoe",          ITEM_FLAG_DAMAGABLE},
	{ITEM_IRON_HOE,           "Iron hoe",           ITEM_FLAG_DAMAGABLE},
	{ITEM_DIAMOND_HOE,        "Diamond hoe",        ITEM_FLAG_DAMAGABLE},
	{ITEM_GOLD_HOE,           "Gold hoe",           ITEM_FLAG_DAMAGABLE},
	{ITEM_LEATHER_CAP,        "Leather cap",        ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_LEATHER_TUNIC,      "Leather tunic",      ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_LEATHER_PANTS,      "Leather pants",      ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_LEATHER_BOOTS,      "Leather boots",      ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_CHAIN_HELMET,       "Chain cap",          ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_CHAIN_CHESTPLATE,   "Chain chestplate",   ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_CHAIN_LEGGINGS,     "Chain leggings",     ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_CHAIN_BOOTS,        "Chain boots",        ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_IRON_HELMET,        "Iron cap",           ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_IRON_CHESTPLATE,    "Iron chestplate",    ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_IRON_LEGGINGS,      "Iron leggings",      ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_IRON_BOOTS,         "Iron boots",         ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_DIAMOND_HELMET,     "Diamond cap",        ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_DIAMOND_CHESTPLATE, "Diamond chestplate", ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_DIAMOND_LEGGINGS,   "Diamond leggings",   ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_DIAMOND_BOOTS,      "Diamond boots",      ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_GOLDEN_HELMET,      "Gold cap",           ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_GOLDEN_CHESTPLATE,  "Gold chestplate",    ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_GOLDEN_LEGGINGS,    "Gold leggings",      ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_GOLDEN_BOOTS,       "Gold boots",         ITEM_FLAG_DAMAGABLE | ITEM_FLAG_ENCHANTABLE},
	{ITEM_FISHING_ROD,        "Fishing rod",        ITEM_FLAG_DAMAGABLE},
	{ITEM_SHEARS,             "Shears",             ITEM_FLAG_DAMAGABLE}
};

static int item_compare(const item_type *id, const struct bedrock_item *item)
{
	if (*id < item->id)
		return -1;
	else if (*id > item->id)
		return 1;
	return 0;
}

typedef int (*compare_func)(const void *, const void *);

struct bedrock_item *item_find(item_type id)
{
	return bsearch(&id, bedrock_items, sizeof(bedrock_items) / sizeof(struct bedrock_item), sizeof(struct bedrock_item), (compare_func) item_compare);
}

struct bedrock_item *item_find_or_create(item_type id)
{
	static struct bedrock_item i;
	struct bedrock_item *item = item_find(id);

	if (item == NULL)
	{
		bedrock_log(LEVEL_DEBUG, "items: Unrecognized item %d", id);

		i.flags = 0;
		i.id = id;
		i.name = "Unknown";
		item = &i;
	}

	return item;
}

#include "util/list.h"
#include "blocks/items.h"

struct recipe
{
	/* width and height of recipe */
	int x, y;
	/* input[y][x] */
	struct item_stack input[3][3];
	struct item_stack output;
};

extern bedrock_list recipies;

extern void recipe_init();
extern void recipe_shutdown();


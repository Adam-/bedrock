#include "crafting/recipe.h"
#include "util/memory.h"
#include "util/yml.h"

#include <sys/types.h>
#include <dirent.h>

bedrock_list recipes = LIST_INIT;

void recipe_init()
{
	DIR *dir = opendir("data/recipes");
	struct dirent *ent;

	recipes.free = bedrock_free;

	if (!dir)
		return;
	
	while ((ent = readdir(dir)))
	{
		char filename[PATH_MAX];
		struct yaml_object *yaml;
		struct recipe *recipe;
		bedrock_node *node;

		if (!strstr(ent->d_name, ".yml"))
			continue;

		snprintf(filename, sizeof(filename), "data/recipes/%s", ent->d_name);

		yaml = yaml_parse(filename);
		if (yaml == NULL)
			continue;

		recipe = bedrock_malloc(sizeof(struct recipe));

		LIST_FOREACH(&yaml->objects, node)
		{
			struct yaml_object *obj = node->data;

			if (!strcmp(obj->name, "recipe"))
			{
				bedrock_node *node2;
				int y;

				y = 0;
				LIST_FOREACH(&obj->objects, node2)
				{
					struct yaml_object *obj2 = node2->data;
					bedrock_node *node3;
					int x;

					if (y > 2)
						break;

					x = 0;
					LIST_FOREACH(&obj2->objects, node3)
					{
						struct yaml_object *obj3 = node3->data;

						if (x > 2)
							break;

						recipe->input[y][x].id = atoi(obj3->value);
						if (recipe->input[y][x].id > 0)
						{
							recipe->input[y][x].count = 1;

							if (x + 1 > recipe->x)
								recipe->x = x + 1;
							if (y + 1 > recipe->y)
								recipe->y = y + 1;
						}

						++x;
					}

					++y;
				}
			}
			else if (!strcmp(obj->name, "output"))
			{
				recipe->output.id = atoi(obj->value);
				if (!recipe->output.count)
					recipe->output.count = 1;
			}
			else if (!strcmp(obj->name, "output_count"))
				recipe->output.count = atoi(obj->value);
		}

		bedrock_list_add(&recipes, recipe);

		yaml_object_free(yaml);
	}

	closedir(dir);

	bedrock_log(LEVEL_DEBUG, "crafting: Loaded %lu recipes.", recipes.count);
}

void recipe_shutdown()
{
	bedrock_list_clear(&recipes);
}


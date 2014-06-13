#include "server/physics.h"
#include "blocks/items.h"
#include "server/packets.h"

/* physics handlers for items */

void physics_item_new_client(struct projectile *p, struct client *client, struct dropped_item *item)
{
	/* item projectile has come within render distance of a client! */
	bedrock_assert(&item->p == p, ;);
	packet_spawn_object_item(client, item);
}

void physics_item_initialize(struct dropped_item *di, float x, float y, float z)
{
	di->p.pos.x = x;
	di->p.pos.y = y;
	di->p.pos.z = z;

	di->p.ag = BEDROCK_ITEM_AG;
	di->p.drag = BEDROCK_ITEM_DRAG;
	di->p.terminal_velocity = BEDROCK_ITEM_TV;

	di->p.on_move_column = (projectile_on_move_column_callback) physics_item_move_column;
	di->p.on_update = projectile_update;
	di->p.on_collide = projectile_collide;
	di->p.on_new_client = (projectile_new_client) physics_item_new_client;
	di->p.on_destroy = projectile_destroy;

	di->p.data = di;
}

void physics_item_move_column(struct projectile *p, struct column *oldc, struct column *newc, struct dropped_item *item)
{
	/* Move across columns */
	bedrock_list_del_node(&oldc->items, &item->node);
	bedrock_list_add_node(&newc->items, &item->node, item);

	projectile_move_column(p, oldc, newc, item);
}


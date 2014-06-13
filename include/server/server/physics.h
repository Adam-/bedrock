#ifndef PHYSICS_H
#define PHYSICS_H

#include "util/list.h"

struct column;
struct world;
struct client;
struct dropped_item;

struct vector
{
	float x;
	float y;
	float z;
};

struct projectile;

typedef void (*projectile_on_move_column_callback)(struct projectile *, struct column *oldc, struct column *newc, void *data);

typedef void (*projectile_on_update_callback)(struct projectile *p, void *data);

typedef void (*projectile_on_collide)(struct projectile *p, uint8_t *block, void *data);

/*
 * called when this projectile comes within reder distance of another client
 */
typedef void (*projectile_new_client)(struct projectile *p, struct client *client, void *data);

typedef void (*projectile_on_destroy)(struct projectile *p, void *data);

struct projectile
{
	bool moving; /* whether or not this is linked in */
	bedrock_node node; /* projectile_list node */
	bedrock_node cnode; /* column->projectiles node */

	uint32_t id; /* entity id */

	struct world *world; /* world this projectile is in */
	struct column *column; /* column this projectile is in */

	struct vector pos; /* position */
	struct vector velocity; /* velocity */
	float ag; /* acceleration due to gravity */
	float terminal_velocity;
	float drag;

	projectile_on_move_column_callback on_move_column;
	projectile_on_update_callback on_update;
	projectile_on_collide on_collide;
	projectile_new_client on_new_client;
	projectile_on_destroy on_destroy;
	void *data; /* opque data associated with this projectile */
};

extern void physics_add(struct column *column, struct projectile *);
extern void physics_process(long ticks);

extern void projectile_update(struct projectile *p, void *data);
extern void projectile_move_column(struct projectile *p, struct column *oldc, struct column *newc, void *data);
extern void projectile_collide(struct projectile *p, uint8_t *block, void *data);
extern void projectile_on_new_client(struct projectile *p, struct client *client, void *data);
extern void projectile_destroy(struct projectile *p, void *data);

extern void physics_item_new_client(struct projectile *p, struct client *client, struct dropped_item *item);
extern void physics_item_initialize(struct dropped_item *di, float x, float y, float z);
extern void physics_item_move_column(struct projectile *p, struct column *oldc, struct column *newc, struct dropped_item *item);

#endif

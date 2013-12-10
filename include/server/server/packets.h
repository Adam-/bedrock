#ifndef BEDROCK_SERVER_PACKETS_H
#define BEDROCK_SERVER_PACKETS_H

#include "server/column.h"

/* packet function prototypes */

extern void packet_send_block_change(struct client *client, int32_t x, uint8_t y, int32_t z, uint16_t id, uint8_t data);

extern int packet_block_placement(struct client *client, bedrock_packet *p);

enum
{
	CHANGE_GAME_MODE = 3
};

extern void packet_send_change_game_state(struct client *client, uint8_t reason, float gamemode);

enum
{
	SPECIAL_CHAR_1 = 0xC2,
	SPECIAL_CHAR_2 = 0xA7
};

enum
{
	COLOR_BLACK        = 0x30, // 0
	COLOR_DARK_BLUE    = 0x31, // 1
	COLOR_DARK_GREEN   = 0x32, // 2
	COLOR_DARK_CYAN    = 0x33, // 3
	COLOR_DARK_RED     = 0x34, // 4
	COLOR_PURPLE       = 0x35, // 5
	COLOR_GOLD         = 0x36, // 6
	COLOR_GREY         = 0x37, // 7
	COLOR_DARK_GREY    = 0x38, // 8
	COLOR_BLUE         = 0x39, // 9
	COLOR_BRIGHT_GREEN = 0x61, // a
	COLOR_CYAN         = 0x62, // b
	COLOR_RED          = 0x63, // c
	COLOR_PINK         = 0x64, // d
	COLOR_YELLOW       = 0x65, // e
	COLOR_WHITE        = 0x66  // f
};

enum
{
	STYLE_RANDOM        = 0x6B, // k
	STYLE_BOLD          = 0x6C, // l
	STYLE_STRIKETHROUGH = 0x6D, // m
	STYLE_UNDERLINED    = 0x6E, // n
	STYLE_ITALIC        = 0x6F, // o
	STYLE_PLAIN         = 0x72  // r
};

extern int packet_chat_message(struct client *client, bedrock_packet *p);
extern void packet_send_chat_message(struct client *client, const char *buf, ...) bedrock_printf(2);

extern int packet_click_window(struct client *client, bedrock_packet *p);

extern int packet_client_settings(struct client *client, bedrock_packet *packet);

extern int packet_client_status(struct client *client, bedrock_packet *p);

extern int packet_close_window(struct client *client, bedrock_packet *p);

extern void packet_send_collect_item(struct client *client, struct dropped_item *di);

#define PACKET_COLUMN_BULK_INIT LIST_INIT

/* List of columns */
typedef bedrock_list packet_column_bulk;

extern void packet_column_bulk_add(struct client *client, packet_column_bulk *columns, struct column *column);
extern void packet_send_column_bulk(struct client *client, packet_column_bulk *columns);


extern void packet_send_column_empty(struct client *client, struct column *column);

extern int packet_confirm_transaction(struct client *client, bedrock_packet *p);
extern void packet_send_confirm_transaction(struct client *client, uint8_t window, int16_t action_number, uint8_t accepted);

extern int packet_creative_inventory_action(struct client *client, bedrock_packet *p);

extern void packet_send_destroy_entity_player(struct client *client, struct client *c);
extern void packet_send_destroy_entity_dropped_item(struct client *client, struct dropped_item *di);

extern int packet_disconnect(struct client *client, bedrock_packet *p);
extern void packet_send_disconnect(struct client *client, const char *reason);

extern void packet_send_encryption_request(struct client *client);

extern int packet_encryption_response(struct client *client, bedrock_packet *p);


extern int packet_entity_action(struct client *client, bedrock_packet *p);

extern int packet_entity_animation(struct client *client, bedrock_packet *p);
extern void packet_send_entity_animation(struct client *client, struct client *target, uint8_t anim);

enum
{
	ENTITY_EQUIPMENT_HELD
};

extern void packet_send_entity_equipment(struct client *client, struct client *c, uint16_t slot, struct item *item, uint16_t damage);

extern void packet_send_entity_head_look(struct client *client, struct client *target);

enum entity_metadata_type
{
	ENTITY_METADATA_TYPE_BYTE,
	ENTITY_METADATA_TYPE_SHORT,
	ENTITY_METADATA_TYPE_INT,
	ENTITY_METADATA_TYPE_FLOAT,
	ENTITY_METADATA_TYPE_STRING,
	ENTITY_METADATA_TYPE_SLOT,
	ENTITY_METADATA_TYPE_3INT
};

enum entity_metadata_index
{
	ENTITY_METADATA_INDEX_FLAGS = 0,
	ENTITY_METADATA_INDEX_DROWNING_COUNTER = 1,
	ENTITY_METADATA_INDEX_POTION_EFFECTS = 8,
	ENTITY_METADATA_INDEX_SLOT = 10,
	ENTITY_METADATA_INDEX_ANIMALS = 12
};

extern void packet_send_entity_metadata(struct client *client, enum entity_metadata_index index, enum entity_metadata_type type, const void *data, size_t size);
extern void packet_send_entity_metadata_slot(struct client *client, struct dropped_item *di);

extern void packet_send_entity_properties(struct client *client, const char *property, double value);

extern void packet_send_entity_teleport(struct client *client, struct client *targ);

extern int packet_handshake(struct client *client, bedrock_packet *p);

extern int packet_held_item_change(struct client *client, bedrock_packet *p);

extern void packet_send_join_game(struct client *client);

extern int packet_keep_alive(struct client *client, bedrock_packet *p);
extern void packet_send_keep_alive(struct client *client, uint32_t id);

extern int packet_login(struct client *client, bedrock_packet *p);
extern void packet_send_login_success(struct client *client);

extern void packet_send_open_window(struct client *client, uint8_t type, const char *title, uint8_t slots);

extern int packet_ping(struct client *client, bedrock_packet *p);

extern int packet_player_abilities(struct client *client, bedrock_packet *packet);

enum
{
	START_DIGGING,
	STOP_DIGGING = 2,
	DROP_ITEM = 4
};

extern int packet_player_digging(struct client *client, bedrock_packet *p);

extern int packet_player(struct client *client, bedrock_packet *p);


extern void packet_send_player_list_item(struct client *client, struct client *c, uint8_t online);

extern int packet_player_look(struct client *client, bedrock_packet *p);

extern int packet_plugin_message(struct client *client, bedrock_packet *p);

extern int packet_position_and_look(struct client *client, bedrock_packet *p);
extern void packet_send_position_and_look(struct client *client);


extern int packet_position(struct client *client, bedrock_packet *p);

extern void packet_send_set_slot(struct client *client, uint8_t window_id, uint16_t slot, struct item *item, uint8_t count, int16_t damage);

extern void packet_spawn_object_item(struct client *client, struct dropped_item *di);

extern void packet_send_spawn_player(struct client *client, struct client *c);

extern void packet_send_spawn_point(struct client *client);

extern int packet_status_request(struct client *client, bedrock_packet *p);

extern void packet_send_time(struct client *client);

enum
{
	WINDOW_PROPERTY_FURNACE_PROGRESS_ARROW,
	WINDOW_PROPERTY_FURNACE_FIRE_ICON
};

extern void packet_send_update_window_property(struct client *client, int8_t window_id, int16_t property, int16_t value);

#endif

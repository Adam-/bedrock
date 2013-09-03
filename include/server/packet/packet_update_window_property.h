
enum
{
	WINDOW_PROPERTY_FURNACE_PROGRESS_ARROW,
	WINDOW_PROPERTY_FURNACE_FIRE_ICON
};

extern void packet_send_update_window_property(struct client *client, int8_t window_id, int16_t property, int16_t value);

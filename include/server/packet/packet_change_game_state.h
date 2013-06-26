
enum
{
	CHANGE_GAME_MODE = 3
};

extern void packet_send_change_game_state(struct client *client, uint8_t reason, uint8_t gamemode);

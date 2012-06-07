
enum
{
	START_DIGGING,
	STOP_DIGGING = 2,
	DROP_ITEM = 4,
	SHOOT_ARROW_FNIISH_EATING = 5
};

extern int packet_player_digging(struct bedrock_client *client, const unsigned char *buffer, size_t len);

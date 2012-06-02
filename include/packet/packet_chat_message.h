
#define SPECIAL_CHAR 0xA7

extern int packet_chat_message(struct bedrock_client *client, const unsigned char *buffer, size_t len);
extern void packet_send_chat_message(struct bedrock_client *client, const char *buf, ...);

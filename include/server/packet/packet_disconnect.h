
extern int packet_disconnect(struct bedrock_client *client, const bedrock_packet *p);
extern void packet_send_disconnect(struct bedrock_client *client, const char *reason);

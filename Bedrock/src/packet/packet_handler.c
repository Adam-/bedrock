#include "packet/packet.h"

int packet_keep_alive(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	uint32_t id;
	size_t offset = 1;

	packet_read_int(buffer, len, &offset, &id, sizeof(id));

	bedrock_client_send(client, &id, sizeof(id));

	bedrock_assert(offset == 5);
	return offset;
}

int packet_login_request(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	return 0;
}

int packet_handshake(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	char username[64];
	packet_read_string(buffer, len, &offset, username, sizeof(username));
	printf("GOT %s\n", username);
}

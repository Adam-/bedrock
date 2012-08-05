#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"

int packet_encryption_response(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	int16_t shared_secret_len, verify_token_len;
	char shared_secret[512], verify_token[512];
	char decrypted_shared_secret[512], decrypted_verify_token[512];
	int decrypted_shared_secret_len;

	packet_read_int(p, &offset, &shared_secret_len, sizeof(shared_secret_len));
	bedrock_assert(shared_secret_len <= sizeof(shared_secret), ;);
	packet_read(p, &offset, shared_secret, shared_secret_len);

	packet_read_int(p, &offset, &verify_token_len, sizeof(verify_token_len));
	bedrock_assert(verify_token_len <= sizeof(verify_token), ;);
	packet_read(p, &offset, verify_token, verify_token_len);

	crypto_rsa_decrypt(verify_token, verify_token_len, decrypted_verify_token, sizeof(decrypted_verify_token));
	if (memcmp(decrypted_verify_token, client->key, BEDROCK_VERIFY_TOKEN_LEN))
	{
		packet_send_disconnect(client, "Invalid verify token");
		return -1;
	}

	decrypted_shared_secret_len = crypto_rsa_decrypt(shared_secret, shared_secret_len, decrypted_shared_secret, sizeof(decrypted_shared_secret));
	bedrock_assert(decrypted_shared_secret_len == BEDROCK_SHARED_SECRET_LEN, ;);

	memcpy(&client->key, decrypted_shared_secret, sizeof(client->key));
	client->authenticated = STATE_LOGGING_IN;

	// Would check session.minecraft.net here
	
	packet_send_encryption_response(client);

	client->authenticated = STATE_LOGGED_IN;

	return offset;
}

void packet_send_encryption_response(struct bedrock_client *client)
{
	bedrock_packet packet;
	uint16_t s = 0;

	packet_init(&packet, ENCRYPTION_RESPONSE);

	packet_pack_header(&packet, ENCRYPTION_RESPONSE);
	packet_pack_int(&packet, &s, sizeof(s));
	packet_pack_int(&packet, &s, sizeof(s));

	client_send_packet(client, &packet);
}


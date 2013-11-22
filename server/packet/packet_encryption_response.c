#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_encryption_response.h"
#include "packet/packet_disconnect.h"
#include "util/crypto.h"

int packet_encryption_response(struct client *client, bedrock_packet *p)
{
	uint16_t shared_secret_len, verify_token_len;
	unsigned char shared_secret[512], verify_token[512];
	unsigned char decrypted_shared_secret[512], decrypted_verify_token[512];
	int decrypted_shared_secret_len;
	static const EVP_CIPHER *cipher;

	packet_read_int(p, &shared_secret_len, sizeof(shared_secret_len));
	if (p->error)
		return p->error;
	if (shared_secret_len > sizeof(shared_secret))
		return ERROR_NOT_ALLOWED;
	packet_read(p, shared_secret, shared_secret_len);

	packet_read_int(p, &verify_token_len, sizeof(verify_token_len));
	if (p->error)
		return p->error;
	if (verify_token_len > sizeof(verify_token))
		return ERROR_NOT_ALLOWED;
	packet_read(p, verify_token, verify_token_len);
	if (p->error)
		return p->error;

	crypto_rsa_decrypt(verify_token, verify_token_len, decrypted_verify_token, sizeof(decrypted_verify_token));
	if (memcmp(decrypted_verify_token, crypto_auth_token(), BEDROCK_VERIFY_TOKEN_LEN))
	{
		packet_send_disconnect(client, "Invalid verify token");
		return ERROR_UNEXPECTED;
	}

	decrypted_shared_secret_len = crypto_rsa_decrypt(shared_secret, shared_secret_len, decrypted_shared_secret, sizeof(decrypted_shared_secret));

	bedrock_assert(decrypted_shared_secret_len == BEDROCK_SHARED_SECRET_LEN, ;);

	/* Initialize contexts using shared secret */
	if (cipher == NULL)
		cipher = crypto_cipher();
	if (!EVP_DecryptInit_ex(&client->in_cipher_ctx, cipher, NULL, decrypted_shared_secret, decrypted_shared_secret))
		bedrock_log(LEVEL_CRIT, "crypto: Unable to initialize encryption context");
	if (!EVP_EncryptInit_ex(&client->out_cipher_ctx, cipher, NULL, decrypted_shared_secret, decrypted_shared_secret))
		bedrock_log(LEVEL_CRIT, "crypto: Unable to initialize encryption context");

	client->state = STATE_LOGGING_IN;

	// Would check session.minecraft.net here
	
	client->state = STATE_LOGGED_IN;

	return ERROR_OK;
}


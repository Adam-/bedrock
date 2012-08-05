#include "server/bedrock.h"
#include "util/memory.h"
#include "config/hard.h"

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/x509.h>

static RSA *keypair;
static unsigned char *pubkey_encoded;
static int pubkey_len;

void crypto_init()
{
	keypair = RSA_generate_key(BEDROCK_CERT_BIT_SIZE, BEDROCK_CERT_EXPONENT, NULL, NULL);
	if (keypair == NULL)
	{
		unsigned long en = ERR_get_error();
		bedrock_log(LEVEL_CRIT, "cert: Unable to generate RSA keypair - %s", ERR_error_string(en, NULL));
		exit(-1);
	}

	BIO *pubkey_bio = BIO_new(BIO_s_mem());
	if (!i2d_RSAPublicKey_bio(pubkey_bio, keypair))
	{
		unsigned long en = ERR_get_error();
		bedrock_log(LEVEL_CRIT, "cert: Unable to write public key to BIO - %s", ERR_error_string(en, NULL));
		exit(-1);
	}

	pubkey_len = BIO_pending(pubkey_bio);
	pubkey_encoded = bedrock_malloc(pubkey_len);
	if (BIO_read(pubkey_bio, pubkey_encoded, pubkey_len) != pubkey_len)
	{
		bedrock_log(LEVEL_CRIT, "cert: Unable to read public key BIO");
		exit(-1);
	}
	BIO_free_all(pubkey_bio);
}

void crypto_shutdown()
{
	RSA_free(keypair);
}

int crypto_pubkey_len()
{
	return pubkey_len;
}

unsigned char *crypto_pubkey()
{
	return pubkey_encoded;
}


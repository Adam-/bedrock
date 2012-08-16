
extern void crypto_init();
extern void crypto_shutdown();
extern int crypto_pubkey_len();
extern unsigned char *crypto_pubkey();
extern unsigned char *crypto_auth_token();
extern const EVP_CIPHER *crypto_cipher();
extern int crypto_rsa_decrypt(const unsigned char *src, size_t src_len, unsigned char *dest, size_t dest_len);
extern int crypto_aes_encrypt(EVP_CIPHER_CTX *context, const unsigned char *src, size_t src_len, unsigned char *dest, size_t dest_len);
extern int crypto_aes_decrypt(EVP_CIPHER_CTX *context, const unsigned char *src, size_t src_len, unsigned char *dest, size_t dest_len);


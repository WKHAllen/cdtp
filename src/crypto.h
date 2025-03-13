/**
 * CDTP crypto utilities.
 */

#pragma once
#ifndef CDTP_CRYPTO_H
#define CDTP_CRYPTO_H

#include "util.h"
#include <stdlib.h>
#include <string.h>

#define BIO void
#define BIO_METHOD void
#define EVP_PKEY void
#define pem_password_cb void
#define OSSL_LIB_CTX void
#define EVP_CIPHER_CTX void
#define EVP_CIPHER void
#define ENGINE void

#define BIO_CTRL_PENDING 10

extern BIO *BIO_new(const BIO_METHOD *type);
extern BIO *BIO_new_mem_buf(const void *buf, int len);
extern const BIO_METHOD *BIO_s_mem(void);
extern long BIO_ctrl(BIO *bp, int cmd, long larg, void *parg);
extern int BIO_read(BIO *b, void *data, int dlen);
extern int BIO_free(BIO *a);
extern void BIO_free_all(BIO *a);
extern EVP_PKEY *PEM_read_bio_PUBKEY(BIO *bp, EVP_PKEY **x, pem_password_cb *cb,
    void *u);
extern EVP_PKEY *PEM_read_bio_PrivateKey(BIO *bp, EVP_PKEY **x,
    pem_password_cb *cb, void *u);
extern int PEM_write_bio_PUBKEY(BIO *bp, EVP_PKEY *x);
extern int PEM_write_bio_PrivateKey(BIO *bp, const EVP_PKEY *x,
    const EVP_CIPHER *enc, unsigned char *kstr,
    int klen, pem_password_cb *cb, void *u);
extern EVP_PKEY *EVP_PKEY_Q_keygen(OSSL_LIB_CTX *libctx, const char *propq,
    const char *type, ...);
extern int EVP_PKEY_get_size(const EVP_PKEY *pkey);
extern void EVP_PKEY_free(EVP_PKEY *key);
extern EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void);
extern int EVP_CIPHER_CTX_get_block_size(const EVP_CIPHER_CTX *ctx);
extern void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *ctx);
extern EVP_CIPHER *EVP_aes_256_cbc(void);
extern int EVP_CIPHER_get_iv_length(const EVP_CIPHER *e);
extern int EVP_SealInit(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
    unsigned char **ek, int *ekl, unsigned char *iv,
    EVP_PKEY **pubk, int npubk);
extern int EVP_SealFinal(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
extern int EVP_OpenInit(EVP_CIPHER_CTX *ctx, EVP_CIPHER *type,
    unsigned char *ek, int ekl, unsigned char *iv,
    EVP_PKEY *priv);
extern int EVP_OpenFinal(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
extern int EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
    ENGINE *impl, const unsigned char *key,
    const unsigned char *iv);
extern int EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
    int *outl, const unsigned char *in, int inl);
extern int EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out,
    int *outl);
extern int EVP_DecryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
    ENGINE *impl, const unsigned char *key,
    const unsigned char *iv);
extern int EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
    const unsigned char *in, int inl);
extern int EVP_DecryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm,
    int *outl);
extern int RAND_bytes(unsigned char *buf, int num);
extern unsigned long ERR_get_error(void);

#define BIO_pending(b) (int)BIO_ctrl(b, BIO_CTRL_PENDING, 0, NULL)
#define EVP_PKEY_size EVP_PKEY_get_size
#define EVP_CIPHER_CTX_block_size EVP_CIPHER_CTX_get_block_size
#define EVP_CIPHER_iv_length EVP_CIPHER_get_iv_length
#define EVP_RSA_gen(bits) \
    EVP_PKEY_Q_keygen(NULL, NULL, "RSA", (size_t)(0 + (bits)))
#define EVP_SealUpdate(a, b, c, d, e) EVP_EncryptUpdate(a, b, c, d, e)
#define EVP_OpenUpdate(a, b, c, d, e) EVP_DecryptUpdate(a, b, c, d, e)

// The RSA key size.
#define CDTP_RSA_KEY_SIZE 2048

// The AES key size.
#define CDTP_AES_KEY_SIZE 32

// The AES nonce size.
#define CDTP_AES_NONCE_SIZE 16

/**
 * Generic data to be encrypted/decrypted.
 */
typedef struct _CDTPCryptoData {
    void *data;
    size_t data_size;
} CDTPCryptoData;

/**
 * An RSA public key.
 */
typedef struct _CDTPRSAPublicKey {
    char *key;
    size_t key_size;
} CDTPRSAPublicKey;

/**
 * An RSA private key.
 */
typedef struct _CDTPRSAPrivateKey {
    char *key;
    size_t key_size;
} CDTPRSAPrivateKey;

/**
 * An RSA key pair.
 */
typedef struct _CDTPRSAKeyPair {
    CDTPRSAPublicKey *public_key;
    CDTPRSAPrivateKey *private_key;
} CDTPRSAKeyPair;

/**
 * An AES key.
 */
typedef struct _CDTPAESKey {
    char *key;
    size_t key_size;
} CDTPAESKey;

/**
 * Create a generic piece of crypto data.
 *
 * @param data The data itself.
 * @param data_size The size of the data, in bytes.
 * @return The new crypto data object.
 *
 * Note that this makes its own copy of `data` and is not responsible for freeing it itself.
 */
CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_data(void *data, size_t data_size);

/**
 * Free the memory used by a piece of crypto data.
 *
 * @param crypto_data The crypto data.
 */
CDTP_TEST_EXPORT void _cdtp_crypto_data_free(CDTPCryptoData *crypto_data);

/**
 * Free the memory used by a piece of crypto data and get the inner data itself.
 *
 * @param crypto_data The crypto data.
 * @return The inner data.
 *
 * Note that `free` will need to be called on the returned data.
 */
CDTP_TEST_EXPORT void *_cdtp_crypto_data_unwrap(CDTPCryptoData *crypto_data);

/**
 * Get a byte representation of an RSA public key.
 *
 * @param public_key The RSA public key.
 * @return The byte representation of the public key.
 */
CDTPCryptoData *_cdtp_crypto_rsa_public_key_to_bytes(CDTPRSAPublicKey *public_key);

/**
 * Get a byte representation of an RSA private key.
 *
 * @param private_key The RSA private key.
 * @return The byte representation of the private key.
 */
CDTPCryptoData *_cdtp_crypto_rsa_private_key_to_bytes(CDTPRSAPrivateKey *private_key);

/**
 * Get a representation of a public key from the public key bytes.
 *
 * @param public_key_bytes The public key bytes.
 * @param public_key_size The size of the public key, in bytes.
 * @return The public key representation.
 *
 * Note that this makes its own copy of `public_key_bytes` and is not responsible for freeing it itself.
 */
CDTPRSAPublicKey *_cdtp_crypto_rsa_public_key_from_bytes(char *public_key_bytes, size_t public_key_size);

/**
 * Get a representation of a private key from the private key bytes.
 *
 * @param private_key_bytes The private key bytes.
 * @param private_key_size The size of the private key, in bytes.
 * @return The private key representation.
 *
 * Note that this makes its own copy of `private_key_bytes` and is not responsible for freeing it itself.
 */
CDTPRSAPrivateKey *_cdtp_crypto_rsa_private_key_from_bytes(char *private_key_bytes, size_t private_key_size);

/**
 * Free the memory used by an RSA public key.
 *
 * @param public_key The RSA public key.
 */
void _cdtp_crypto_rsa_public_key_free(CDTPRSAPublicKey *public_key);

/**
 * Free the memory used by an RSA private key.
 *
 * @param private_key The RSA private key.
 */
void _cdtp_crypto_rsa_private_key_free(CDTPRSAPrivateKey *private_key);

/**
 * Generate an RSA key pair.
 *
 * @return The generated key pair.
 */
CDTP_TEST_EXPORT CDTPRSAKeyPair *_cdtp_crypto_rsa_key_pair(void);

/**
 * Free the memory used by an RSA key pair.
 *
 * @param key_pair The RSA key pair.
 */
CDTP_TEST_EXPORT void _cdtp_crypto_rsa_key_pair_free(CDTPRSAKeyPair *key_pair);

/**
 * Encrypt data with RSA.
 *
 * @param public_key The RSA public key.
 * @param plaintext The data to encrypt.
 * @param plaintext_size The size of the data, in bytes.
 * @return A representation of the encrypted data.
 */
CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_rsa_encrypt(CDTPRSAPublicKey *public_key, void *plaintext, size_t plaintext_size);

/**
 * Decrypt data with RSA.
 *
 * @param private_key The RSA private key.
 * @param ciphertext The data to decrypt.
 * @param ciphertext_size The size of the data, in bytes.
 * @return A representation of the decrypted data.
 */
CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_rsa_decrypt(CDTPRSAPrivateKey *private_key, void *ciphertext, size_t ciphertext_size);

/**
 * Generate an AES key.
 *
 * @return The generated key.
 */
CDTP_TEST_EXPORT CDTPAESKey *_cdtp_crypto_aes_key(void);

/**
 * Free the memory used by an AES key.
 *
 * @param key The AES key.
 */
CDTP_TEST_EXPORT void _cdtp_crypto_aes_key_free(CDTPAESKey *key);

/**
 * Create an AES key from bytes.
 *
 * @param bytes The key data.
 * @param size The size of the key data, in bytes.
 * @return The AES key.
 */
CDTP_TEST_EXPORT CDTPAESKey *_cdtp_crypto_aes_key_from(char *bytes, size_t size);

/**
 * Encrypt data with AES.
 *
 * @param key The AES key.
 * @param plaintext The data to encrypt.
 * @param plaintext_size The size of the data, in bytes.
 * @return A representation of the encrypted data.
 */
CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_aes_encrypt(CDTPAESKey *key, void *plaintext, size_t plaintext_size);

/**
 * Decrypt data with AES.
 *
 * @param key The AES key.
 * @param ciphertext The data to decrypt.
 * @param ciphertext_size The size of the data, in bytes.
 * @return A representation of the decrypted data.
 */
CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_aes_decrypt(CDTPAESKey *key, void *ciphertext, size_t ciphertext_size);

#endif // CDTP_CRYPTO_H

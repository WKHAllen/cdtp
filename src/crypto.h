/**
 * CDTP crypto utilities.
 */

#pragma once
#ifndef CDTP_CRYPTO_H
#define CDTP_CRYPTO_H

#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/err.h>

// The RSA key size.
#define CDTP_RSA_KEY_SIZE 2048

// The AES key size.
#define CDTP_AES_KEY_SIZE 32

// The AES IV size.
#define CDTP_AES_IV_SIZE 16

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
 * An AES key and IV.
 */
typedef struct _CDTPAESKeyIV {
    char *key;
    size_t key_size;
    char *iv;
    size_t iv_size;
} CDTPAESKeyIV;

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
 * Generate an AES key and IV.
 *
 * @return The generated key and IV.
 */
CDTP_TEST_EXPORT CDTPAESKeyIV *_cdtp_crypto_aes_key_iv(void);

/**
 * Free the memory used by an AES key and IV.
 *
 * @param key_iv The AES key and IV.
 */
CDTP_TEST_EXPORT void _cdtp_crypto_aes_key_iv_free(CDTPAESKeyIV *key_iv);

/**
 * Combine the AES key and IV into a single piece of data. This does not free the memory used by the key and IV.
 *
 * @param key_iv The AES key and IV.
 * @return The combined key and IV.
 */
CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_aes_key_iv_to_data(CDTPAESKeyIV *key_iv);

/**
 * Get an AES key and IV from a single piece of data. This does not free the memory used by the data itself.
 *
 * @param key_iv_data The combined AES key and IV data.
 * @return The AES key and IV.
 */
CDTP_TEST_EXPORT CDTPAESKeyIV *_cdtp_crypto_aes_key_iv_from_data(CDTPCryptoData *key_iv_data);

/**
 * Encrypt data with AES.
 *
 * @param key_iv The AES key and IV.
 * @param plaintext The data to encrypt.
 * @param plaintext_size The size of the data, in bytes.
 * @return A representation of the encrypted data.
 */
CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_aes_encrypt(CDTPAESKeyIV *key_iv, void *plaintext, size_t plaintext_size);

/**
 * Decrypt data with AES.
 *
 * @param key_iv The AES key and IV.
 * @param ciphertext The data to decrypt.
 * @param ciphertext_size The size of the data, in bytes.
 * @return A representation of the decrypted data.
 */
CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_aes_decrypt(CDTPAESKeyIV *key_iv, void *ciphertext, size_t ciphertext_size);

#endif // CDTP_CRYPTO_H

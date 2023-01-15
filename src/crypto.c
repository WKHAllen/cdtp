#include "crypto.h"

CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_data(void *data, size_t data_size)
{
    CDTPCryptoData *crypto_data = (CDTPCryptoData *) malloc(sizeof(CDTPCryptoData));

    crypto_data->data = malloc(data_size);
    memcpy(crypto_data->data, data, data_size);
    crypto_data->data_size = data_size;

    return crypto_data;
}

CDTP_TEST_EXPORT void _cdtp_crypto_data_free(CDTPCryptoData *crypto_data)
{
    free(crypto_data->data);
    free(crypto_data);
}

CDTP_TEST_EXPORT void *_cdtp_crypto_data_unwrap(CDTPCryptoData *crypto_data)
{
    void *data = crypto_data->data;
    free(crypto_data);

    return data;
}

/**
 * Pad a section of bytes to ensure its size is never a multiple of 16 bytes. This alters the data in-place.
 *
 * @param crypto_data The data to pad.
 */
void _cdtp_crypto_pad_data(CDTPCryptoData *crypto_data)
{
    char *padded_data;

    if ((crypto_data->data_size + 1) % 16 == 0) {
        padded_data = malloc(crypto_data->data_size + 2);
        padded_data[0] = (char) 1;
        padded_data[1] = (char) 255;
        memcpy(padded_data + 2, crypto_data->data, crypto_data->data_size + 2);
        crypto_data->data_size += 2;
    } else {
        padded_data = malloc(crypto_data->data_size + 1);
        padded_data[0] = (char) 0;
        memcpy(padded_data + 1, crypto_data->data, crypto_data->data_size + 1);
        crypto_data->data_size += 1;
    }

    free(crypto_data->data);
    crypto_data->data = (void *) padded_data;
}

/**
 * Unpad a section of padded bytes. This alters the data in-place.
 *
 * @param crypto_data The data to unpad.
 */
void _cdtp_crypto_unpad_data(CDTPCryptoData *crypto_data)
{
    char *unpadded_data;

    if (((char *) (crypto_data->data))[0] == ((char) 1)) {
        unpadded_data = malloc(crypto_data->data_size - 2);
        memcpy(unpadded_data, ((char *) (crypto_data->data)) + 2, crypto_data->data_size - 2);
        crypto_data->data_size -= 2;
    } else {
        unpadded_data = malloc(crypto_data->data_size - 1);
        memcpy(unpadded_data, ((char *) (crypto_data->data)) + 1, crypto_data->data_size - 1);
        crypto_data->data_size -= 1;
    }

    free(crypto_data->data);
    crypto_data->data = (void *) unpadded_data;
}

/**
 * Get an OpenSSL representation of a public key.
 *
 * @param public_key The public key.
 * @return The OpenSSL representation of the public key.
 */
EVP_PKEY *_cdtp_crypto_openssl_rsa_public_key(CDTPRSAPublicKey *public_key)
{
    const char *pub_key = public_key->key;
    int pub_len = public_key->key_size;

    BIO *pbkeybio = NULL;

    if ((pbkeybio = BIO_new_mem_buf((const void *) pub_key, pub_len)) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    EVP_PKEY *pb_rsa = NULL;

    if ((pb_rsa = PEM_read_bio_PUBKEY(pbkeybio, &pb_rsa, NULL, NULL)) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    BIO_free(pbkeybio);

    return pb_rsa;
}

/**
 * Get an OpenSSL representation of a private key.
 *
 * @param private_key The private key.
 * @return The OpenSSL representation of the private key.
 */
EVP_PKEY *_cdtp_crypto_openssl_rsa_private_key(CDTPRSAPrivateKey *private_key)
{
    const char *pri_key = private_key->key;
    int pri_len = private_key->key_size;

    BIO *prkeybio = NULL;

    if ((prkeybio = BIO_new_mem_buf((const void *) pri_key, pri_len)) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    EVP_PKEY *p_rsa = NULL;

    if ((p_rsa = PEM_read_bio_PrivateKey(prkeybio, &p_rsa, NULL, NULL)) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    BIO_free(prkeybio);

    return p_rsa;
}

/**
 * Free the memory used by the OpenSSL public key.
 *
 * @param public_key The OpenSSL public key.
 */
void _cdtp_crypto_openssl_rsa_public_key_free(EVP_PKEY *public_key)
{
    EVP_PKEY_free(public_key);
}

/**
 * Free the memory used by the OpenSSL private key.
 *
 * @param private_key The OpenSSL private key.
 */
void _cdtp_crypto_openssl_rsa_private_key_free(EVP_PKEY *private_key)
{
    EVP_PKEY_free(private_key);
}

CDTPRSAPublicKey *_cdtp_crypto_rsa_public_key_from_bytes(char *public_key_bytes, size_t public_key_size)
{
    CDTPRSAPublicKey *public_key = (CDTPRSAPublicKey *) malloc(sizeof(CDTPRSAPublicKey));

    public_key->key = (char *) malloc(public_key_size * sizeof(char));
    memcpy(public_key->key, public_key_bytes, public_key_size);
    public_key->key_size = public_key_size;

    return public_key;
}

CDTPRSAPrivateKey *_cdtp_crypto_rsa_private_key_from_bytes(char *private_key_bytes, size_t private_key_size)
{
    CDTPRSAPrivateKey *private_key = (CDTPRSAPrivateKey *) malloc(sizeof(CDTPRSAPrivateKey));

    private_key->key = (char *) malloc(private_key_size * sizeof(char));
    memcpy(private_key->key, private_key_bytes, private_key_size);
    private_key->key_size = private_key_size;

    return private_key;
}

void _cdtp_crypto_rsa_public_key_free(CDTPRSAPublicKey *public_key)
{
    free(public_key->key);
    free(public_key);
}

void _cdtp_crypto_rsa_private_key_free(CDTPRSAPrivateKey *private_key)
{
    free(private_key->key);
    free(private_key);
}

CDTP_TEST_EXPORT CDTPRSAKeyPair *_cdtp_crypto_rsa_key_pair(void)
{
    EVP_PKEY *r;

    if ((r = EVP_RSA_gen((unsigned int) CDTP_RSA_KEY_SIZE)) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    BIO *bp_public;
    BIO *bp_private;

    if ((bp_public = BIO_new(BIO_s_mem())) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    if (PEM_write_bio_PUBKEY(bp_public, r) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    if ((bp_private = BIO_new(BIO_s_mem())) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    if (PEM_write_bio_PrivateKey(bp_private, r, NULL, NULL, 0, NULL, NULL) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    size_t pub_len = BIO_pending(bp_public);
    size_t pri_len = BIO_pending(bp_private);
    char *public_key_bytes = (char *) malloc(pub_len * sizeof(char));
    char *private_key_bytes = (char *) malloc(pri_len * sizeof(char));

    if (BIO_read(bp_public, public_key_bytes, pub_len) < 1) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    if (BIO_read(bp_private, private_key_bytes, pri_len) < 1) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    BIO_free_all(bp_public);
    BIO_free_all(bp_private);
    EVP_PKEY_free(r);

    CDTPRSAKeyPair *key_pair = (CDTPRSAKeyPair *) malloc(sizeof(CDTPRSAKeyPair));

    key_pair->public_key = _cdtp_crypto_rsa_public_key_from_bytes(public_key_bytes, pub_len);
    key_pair->private_key = _cdtp_crypto_rsa_private_key_from_bytes(private_key_bytes, pri_len);

    free(public_key_bytes);
    free(private_key_bytes);

    return key_pair;
}

CDTP_TEST_EXPORT void _cdtp_crypto_rsa_key_pair_free(CDTPRSAKeyPair *key_pair)
{
    _cdtp_crypto_rsa_public_key_free(key_pair->public_key);
    _cdtp_crypto_rsa_private_key_free(key_pair->private_key);
    free(key_pair);
}

CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_rsa_encrypt(CDTPRSAPublicKey *public_key, void *plaintext, size_t plaintext_size)
{
    CDTPCryptoData *plaintext_padded = _cdtp_crypto_data(plaintext, plaintext_size);
    _cdtp_crypto_pad_data(plaintext_padded);
    unsigned char *plaintext_data = (unsigned char *) plaintext_padded->data;
    int plaintext_len = (int) plaintext_padded->data_size;

    EVP_PKEY *evp_public_key = _cdtp_crypto_openssl_rsa_public_key(public_key);

    int encrypted_key_len;

    int iv_len = EVP_CIPHER_iv_length(EVP_aes_256_cbc());
    unsigned char *iv = (unsigned char *) malloc(iv_len * sizeof(unsigned char));

    if ((encrypted_key_len = EVP_PKEY_size(evp_public_key)) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    unsigned char *encrypted_key = (unsigned char *) malloc(encrypted_key_len * sizeof(unsigned char));

    EVP_CIPHER_CTX *ctx;
    int ciphertext_len;
    int len;

    if ((ctx = EVP_CIPHER_CTX_new()) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    if (EVP_SealInit(ctx, EVP_aes_256_cbc(), &encrypted_key, &encrypted_key_len, iv, &evp_public_key, 1) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    int block_size = EVP_CIPHER_CTX_block_size(ctx);
    unsigned char *ciphertext_unsigned = (unsigned char *) malloc((plaintext_len + block_size - 1) * sizeof(unsigned char));

    len = plaintext_len + block_size - 1;

    if (EVP_SealUpdate(ctx, ciphertext_unsigned, &len, plaintext_data, plaintext_len) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    ciphertext_len = len;

    if (EVP_SealFinal(ctx, ciphertext_unsigned + len, &len) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    ciphertext_len += len;
    ciphertext_unsigned = realloc(ciphertext_unsigned, (size_t) ciphertext_len);

    unsigned char *all_unsigned = (unsigned char *) malloc((CDTP_LENSIZE + encrypted_key_len + iv_len + ciphertext_len) * sizeof(unsigned char));
    unsigned char *encoded_encrypted_key_len = _cdtp_encode_message_size((size_t) encrypted_key_len);
    memcpy(all_unsigned, encoded_encrypted_key_len, CDTP_LENSIZE);
    memcpy(all_unsigned + CDTP_LENSIZE, encrypted_key, encrypted_key_len);
    memcpy(all_unsigned + CDTP_LENSIZE + encrypted_key_len, iv, iv_len);
    memcpy(all_unsigned + CDTP_LENSIZE + encrypted_key_len + iv_len, ciphertext_unsigned, ciphertext_len);

    EVP_CIPHER_CTX_free(ctx);
    _cdtp_crypto_openssl_rsa_public_key_free(evp_public_key);

    CDTPCryptoData *ciphertext = _cdtp_crypto_data((void *) all_unsigned, CDTP_LENSIZE + encrypted_key_len + iv_len + ciphertext_len);

    _cdtp_crypto_data_free(plaintext_padded);
    free(iv);
    free(encrypted_key);
    free(ciphertext_unsigned);
    free(all_unsigned);
    free(encoded_encrypted_key_len);

    return ciphertext;
}

CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_rsa_decrypt(CDTPRSAPrivateKey *private_key, void *ciphertext, size_t ciphertext_size)
{
    EVP_PKEY *evp_private_key = _cdtp_crypto_openssl_rsa_private_key(private_key);
    int iv_len = EVP_CIPHER_iv_length(EVP_aes_256_cbc());

    unsigned char *all_unsigned = (unsigned char *) ciphertext;

    char *encoded_encrypted_key_len = (char *) malloc(CDTP_LENSIZE * sizeof(char));
    memcpy(encoded_encrypted_key_len, all_unsigned, CDTP_LENSIZE);

    int encrypted_key_len = (int) _cdtp_decode_message_size((unsigned char *) encoded_encrypted_key_len);

    unsigned char *encrypted_key = (unsigned char *) malloc(encrypted_key_len * sizeof(unsigned char));
    memcpy(encrypted_key, all_unsigned + CDTP_LENSIZE, encrypted_key_len);

    unsigned char *iv = (unsigned char *) malloc(iv_len * sizeof(unsigned char *));
    memcpy(iv, all_unsigned + CDTP_LENSIZE + encrypted_key_len, iv_len);

    int ciphertext_len = ciphertext_size - (CDTP_LENSIZE + encrypted_key_len + iv_len);

    unsigned char *ciphertext_unsigned = (unsigned char *) malloc(ciphertext_len * sizeof(unsigned char));
    memcpy(ciphertext_unsigned, all_unsigned + CDTP_LENSIZE + encrypted_key_len + iv_len, ciphertext_len);

    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    if ((ctx = EVP_CIPHER_CTX_new()) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    if (EVP_OpenInit(ctx, EVP_aes_256_cbc(), encrypted_key, encrypted_key_len, iv, evp_private_key) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    unsigned char *plaintext_unsigned = (unsigned char *) malloc(ciphertext_len * sizeof(unsigned char));

    if (EVP_OpenUpdate(ctx, plaintext_unsigned, &len, ciphertext_unsigned, ciphertext_len) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    plaintext_len = len;

    if (EVP_OpenFinal(ctx, plaintext_unsigned + len, &len) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    plaintext_len += len;
    plaintext_unsigned = realloc(plaintext_unsigned, plaintext_len);

    CDTPCryptoData *plaintext = _cdtp_crypto_data(plaintext_unsigned, plaintext_len);
    _cdtp_crypto_unpad_data(plaintext);

    EVP_CIPHER_CTX_free(ctx);
    _cdtp_crypto_openssl_rsa_private_key_free(evp_private_key);

    free(encoded_encrypted_key_len);
    free(encrypted_key);
    free(iv);
    free(ciphertext_unsigned);
    free(plaintext_unsigned);

    return plaintext;
}

CDTP_TEST_EXPORT CDTPAESKeyIV *_cdtp_crypto_aes_key_iv(void)
{
    int num_rounds = 5;
    unsigned char key_unsigned[CDTP_AES_KEY_SIZE];
    unsigned char iv_unsigned[CDTP_AES_IV_SIZE];
    unsigned char key_data[CDTP_AES_KEY_SIZE];

    if (RAND_bytes(key_data, CDTP_AES_KEY_SIZE) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    if (EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), NULL, key_data, CDTP_AES_KEY_SIZE, num_rounds, key_unsigned, iv_unsigned) != CDTP_AES_KEY_SIZE) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    CDTPAESKeyIV *key_iv = (CDTPAESKeyIV *) malloc(sizeof(CDTPAESKeyIV));

    key_iv->key = (char *) malloc(CDTP_AES_KEY_SIZE * sizeof(char));
    memcpy(key_iv->key, key_unsigned, CDTP_AES_KEY_SIZE);
    key_iv->key_size = CDTP_AES_KEY_SIZE;

    key_iv->iv = (char *) malloc(CDTP_AES_IV_SIZE * sizeof(char));
    memcpy(key_iv->iv, iv_unsigned, CDTP_AES_IV_SIZE);
    key_iv->iv_size = CDTP_AES_IV_SIZE;

    return key_iv;
}

CDTP_TEST_EXPORT void _cdtp_crypto_aes_key_iv_free(CDTPAESKeyIV *key_iv)
{
    free(key_iv->key);
    free(key_iv->iv);
    free(key_iv);
}

CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_aes_key_iv_to_data(CDTPAESKeyIV *key_iv)
{
    char *combined_bytes = (char *) malloc((key_iv->key_size + key_iv->iv_size) * sizeof(char));
    memcpy(combined_bytes, key_iv->key, key_iv->key_size);
    memcpy(combined_bytes + key_iv->key_size, key_iv->iv, key_iv->iv_size);

    CDTPCryptoData *combined = _cdtp_crypto_data(combined_bytes, key_iv->key_size + key_iv->iv_size);

    free(combined_bytes);

    return combined;
}

CDTP_TEST_EXPORT CDTPAESKeyIV *_cdtp_crypto_aes_key_iv_from_data(CDTPCryptoData *key_iv_data)
{
    CDTPAESKeyIV *key_iv = (CDTPAESKeyIV *) malloc(sizeof(CDTPAESKeyIV));

    key_iv->key = (char *) malloc(CDTP_AES_KEY_SIZE * sizeof(char));
    memcpy(key_iv->key, key_iv_data->data, CDTP_AES_KEY_SIZE);
    key_iv->key_size = CDTP_AES_KEY_SIZE;

    key_iv->iv = (char *) malloc(CDTP_AES_IV_SIZE * sizeof(char));
    memcpy(key_iv->iv, ((char *) (key_iv_data->data)) + CDTP_AES_KEY_SIZE, CDTP_AES_IV_SIZE);
    key_iv->iv_size = CDTP_AES_IV_SIZE;

    return key_iv;
}

CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_aes_encrypt(CDTPAESKeyIV *key_iv, void *plaintext, size_t plaintext_size)
{
    unsigned char *key = (unsigned char *) key_iv->key;
    unsigned char *iv = (unsigned char *) key_iv->iv;
    CDTPCryptoData *plaintext_padded = _cdtp_crypto_data(plaintext, plaintext_size);
    _cdtp_crypto_pad_data(plaintext_padded);
    unsigned char *plaintext_data = (unsigned char *) plaintext_padded->data;
    int plaintext_len = (int) plaintext_padded->data_size;

    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    if ((ctx = EVP_CIPHER_CTX_new()) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    int block_size = EVP_CIPHER_CTX_block_size(ctx);
    unsigned char *ciphertext_unsigned = (unsigned char *) malloc((plaintext_len + block_size - 1) * sizeof(unsigned char));

    len = plaintext_len + block_size - 1;

    if (EVP_EncryptUpdate(ctx, ciphertext_unsigned, &len, plaintext_data, plaintext_len) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext_unsigned + len, &len) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    ciphertext_len += len;
    ciphertext_unsigned = realloc(ciphertext_unsigned, ciphertext_len);
    CDTPCryptoData *ciphertext = _cdtp_crypto_data((void *) ciphertext_unsigned, ciphertext_len);

    EVP_CIPHER_CTX_free(ctx);

    _cdtp_crypto_data_free(plaintext_padded);
    free(ciphertext_unsigned);

    return ciphertext;
}

CDTP_TEST_EXPORT CDTPCryptoData *_cdtp_crypto_aes_decrypt(CDTPAESKeyIV *key_iv, void *ciphertext, size_t ciphertext_size)
{
    unsigned char *key = (unsigned char *) key_iv->key;
    unsigned char *iv = (unsigned char *) key_iv->iv;
    unsigned char *ciphertext_data = (unsigned char *) ciphertext;
    int ciphertext_len = (int) ciphertext_size;

    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    if ((ctx = EVP_CIPHER_CTX_new()) == NULL) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    unsigned char *plaintext_unsigned = (unsigned char *) malloc(ciphertext_len * sizeof(unsigned char));

    if (EVP_DecryptUpdate(ctx, plaintext_unsigned, &len, ciphertext_data, ciphertext_len) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext_unsigned + len, &len) == 0) {
        _cdtp_set_error(CDTP_OPENSSL_ERROR, ERR_get_error());
        return NULL;
    }

    plaintext_len += len;
    plaintext_unsigned = realloc(plaintext_unsigned, plaintext_len);
    CDTPCryptoData *plaintext = _cdtp_crypto_data((void *) plaintext_unsigned, plaintext_len);
    _cdtp_crypto_unpad_data(plaintext);

    EVP_CIPHER_CTX_free(ctx);

    free(plaintext_unsigned);

    return plaintext;
}

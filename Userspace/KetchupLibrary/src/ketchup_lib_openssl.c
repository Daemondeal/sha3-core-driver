#include "./ketchup_lib.h"

#include <openssl/evp.h>
#include <openssl/evperr.h>
#include <openssl/types.h>

#if KETCHUP_LIB_MODE != KETCHUP_LIB_MODE_OPENSSL 
// TODO: Print a better error message
#error "INVALID LIB MODE"
#endif

kc_error kc_sha3_512_init(kc_sha3_context *context) {
    context->algorithm = EVP_sha3_512();
    context->openssl_context = EVP_MD_CTX_create();
    context->digest_length = 512 / 8;

    EVP_DigestInit(context->openssl_context, context->algorithm);

    return KC_ERR_NONE;
}

kc_error kc_sha3_384_init(kc_sha3_context *context) {
    context->algorithm = EVP_sha3_384();
    context->openssl_context = EVP_MD_CTX_create();
    context->digest_length = 384 / 8;

    EVP_DigestInit(context->openssl_context, context->algorithm);

    return KC_ERR_NONE;
}

kc_error kc_sha3_256_init(kc_sha3_context *context) {
    context->algorithm = EVP_sha3_256();
    context->openssl_context = EVP_MD_CTX_create();
    context->digest_length = 256 / 8;

    EVP_DigestInit(context->openssl_context, context->algorithm);

    return KC_ERR_NONE;
}

kc_error kc_sha3_224_init(kc_sha3_context *context) {
    context->algorithm = EVP_sha3_224();
    context->openssl_context = EVP_MD_CTX_create();
    context->digest_length = 224 / 8;

    EVP_DigestInit(context->openssl_context, context->algorithm);

    return KC_ERR_NONE;
}

void kc_sha3_update(kc_sha3_context *context, void const *new_data, uint32_t new_data_length) {
    EVP_DigestUpdate(context->openssl_context, new_data, new_data_length);
}


void kc_sha3_final(kc_sha3_context *context, uint8_t *digest, uint32_t *digest_length) {
    EVP_DigestFinal(context->openssl_context, digest, digest_length);
    EVP_DigestInit(context->openssl_context, context->algorithm);
}

kc_error kc_sha3_close(kc_sha3_context *context) {
    EVP_MD_CTX_free(context->openssl_context);

    return KC_ERR_NONE;
}

kc_error kc_sha3_512(void const *data, uint32_t data_length, uint8_t *digest, uint32_t *digest_length) {
    kc_sha3_context context;
    kc_error error;

    error = kc_sha3_512_init(&context);
    if (error != KC_ERR_NONE) {
        return error;
    }

    kc_sha3_update(&context, data, data_length);
    kc_sha3_final(&context, digest, digest_length);

    return kc_sha3_close(&context);
}

kc_error kc_sha3_384(void const *data, uint32_t data_length, uint8_t *digest, uint32_t *digest_length) {
    kc_sha3_context context;
    kc_error error;

    error = kc_sha3_384_init(&context);
    if (error != KC_ERR_NONE) {
        return error;
    }

    kc_sha3_update(&context, data, data_length);
    kc_sha3_final(&context, digest, digest_length);

    return kc_sha3_close(&context);
}

kc_error kc_sha3_256(void const *data, uint32_t data_length, uint8_t *digest, uint32_t *digest_length) {
    kc_sha3_context context;
    kc_error error;

    error = kc_sha3_256_init(&context);
    if (error != KC_ERR_NONE) {
        return error;
    }

    kc_sha3_update(&context, data, data_length);
    kc_sha3_final(&context, digest, digest_length);

    return kc_sha3_close(&context);
}

kc_error kc_sha3_224(void const *data, uint32_t data_length, uint8_t *digest, uint32_t *digest_length) {
    kc_sha3_context context;
    kc_error error;

    error = kc_sha3_224_init(&context);
    if (error != KC_ERR_NONE) {
        return error;
    }

    kc_sha3_update(&context, data, data_length);
    kc_sha3_final(&context, digest, digest_length);

    return kc_sha3_close(&context);
}

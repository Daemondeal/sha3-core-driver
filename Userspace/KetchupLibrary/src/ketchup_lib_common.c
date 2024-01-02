#include "./ketchup_lib.h"

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
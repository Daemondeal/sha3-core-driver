#ifndef _KETCHTUP_LIB_H
#define _KETCHTUP_LIB_H

#include <stdint.h>

#define KETCHUP_LIB_MODE_HARDWARE 0
#define KETCHUP_LIB_MODE_OPENSSL  1

#ifndef KETCHUP_LIB_MODE
#define KETCHUP_LIB_MODE KETCHUP_LIB_MODE_OPENSSL
#endif

#if KETCHUP_LIB_MODE == KETCHUP_LIB_MODE_HARDWARE
struct kc_sha3_context_s {
    int fd;
    uint32_t digest_length;
};
#endif

#if KETCHUP_LIB_MODE == KETCHUP_LIB_MODE_OPENSSL 
#include <openssl/evp.h>
struct kc_sha3_context_s {
    EVP_MD const *algorithm;
    EVP_MD_CTX *openssl_context;
    uint32_t digest_length;
};
#endif

#define KC_MAX_MD_SIZE 64

typedef enum kc_sha3_error_e {
    KC_ERR_NONE,
    KC_ERR_BUSY,
    KC_ERR_UNSUPPORTED_SIZE,
    // NOTE: This is not ideal, remove this after
    //       the driver is fully defined and 
    //       all possible errors have been enumerated
    KC_ERR_OTHER 
} kc_error;

typedef struct kc_sha3_context_s kc_sha3_context;

kc_error kc_sha3_512_init(kc_sha3_context *context);
kc_error kc_sha3_384_init(kc_sha3_context *context);
kc_error kc_sha3_256_init(kc_sha3_context *context);
kc_error kc_sha3_224_init(kc_sha3_context *context);

void kc_sha3_update(kc_sha3_context *context, void const *new_data, uint32_t new_data_length);
void kc_sha3_final(kc_sha3_context *context, uint8_t *digest, uint32_t *digest_length);

kc_error kc_sha3_close(kc_sha3_context *context);

// Utilities for quickly hashing inputs
kc_error kc_sha3_512(void const *data, uint32_t data_length, uint8_t *digest, uint32_t *digest_length);
kc_error kc_sha3_384(void const *data, uint32_t data_length, uint8_t *digest, uint32_t *digest_length);
kc_error kc_sha3_256(void const *data, uint32_t data_length, uint8_t *digest, uint32_t *digest_length);
kc_error kc_sha3_224(void const *data, uint32_t data_length, uint8_t *digest, uint32_t *digest_length);



#endif // _KETCHTUP_LIB_H

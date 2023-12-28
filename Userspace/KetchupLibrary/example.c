#include <stdio.h>
#include <string.h>

#include "./src/ketchup_lib.h"

// This is an example usage of the library

int main() {
    kc_sha3_context context;
    // Digests are guaranteed to be of size equal or less than KC_MAX_MD_SIZE
    uint8_t digest[KC_MAX_MD_SIZE];
    uint32_t digest_length;
    char test_string[] = "Hello World!";

    // You first need to initialize the context
    kc_sha3_512_init(&context);

    // Then you give it data. Note that you can also call kc_sha3_update repeatedly to give it more data
    kc_sha3_update(&context, (uint8_t*)test_string, strlen(test_string));

    // When you're finished, you can extract the digest from the context
    // After this is done, the context will also be reset, so you can start
    // generating a new hash right ahead.
    kc_sha3_final(&context, digest, &digest_length);

    // Always remember to clean up!
    kc_sha3_close(&context);

    printf("Sha3-512(\"%s\") = ", test_string);
    for (int i = 0; i < digest_length; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");


}
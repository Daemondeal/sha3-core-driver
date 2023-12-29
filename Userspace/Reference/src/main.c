#include "../../KetchupLibrary/src/ketchup_lib.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>


void hash_then_print(kc_sha3_context context, char const *data, size_t data_length) {
    uint8_t digest[KC_MAX_MD_SIZE] = {0};
    uint32_t digest_length;


    kc_sha3_update(&context, data, data_length);
    kc_sha3_final(&context, digest, &digest_length);

    printf("Sha3-%d(\"%s\") = ", digest_length * 8, data);
    for (int i = 0; i < digest_length; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");
}

int main() {
    kc_sha3_context contexts[4];

    kc_sha3_224_init(&contexts[0]);
    kc_sha3_256_init(&contexts[1]);
    kc_sha3_384_init(&contexts[2]);
    kc_sha3_512_init(&contexts[3]);

    char test0[] = "";
    char test1[] = "Hello World";
    char test2[] = "Hello World!";
    char test3[] = "Hello";
    char test4[] = "hi mom";
    char test5[] = "This is a very very very very very very long test test test test test!";

    printf("OpenSSL tests:\n");
    for (int i = 0; i < 4; i++) {
        hash_then_print(contexts[i], test0, strlen(test0));
        hash_then_print(contexts[i], test1, strlen(test1));
        hash_then_print(contexts[i], test2, strlen(test2));
        hash_then_print(contexts[i], test3, strlen(test3));
        hash_then_print(contexts[i], test4, strlen(test4));
        hash_then_print(contexts[i], test5, strlen(test5));
        printf("\n");
    }

    return 0;
}

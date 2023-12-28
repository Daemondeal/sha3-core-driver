#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../../KetchupLibrary/src/ketchup_lib.h"

#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
    kc_sha3_context context;
    uint8_t digest[KC_MAX_MD_SIZE];
    uint8_t buffer[BUFFER_SIZE];
    uint32_t digest_length;
    size_t bytes_read;


    if (argc < 2) {
        printf("USAGE: sha3hash {224|256|384|512} [input]\n");
        return -EINVAL;
    }

    if (strcmp(argv[1], "224") == 0) {
        kc_sha3_224_init(&context);
    } else if (strcmp(argv[1], "256") == 0) {
        kc_sha3_256_init(&context);
    } else if (strcmp(argv[1], "384") == 0) {
        kc_sha3_384_init(&context);
    } else if (strcmp(argv[1], "512") == 0) {
        kc_sha3_512_init(&context);
    } else {
        printf("Invalid hash size.\n");
        return -EINVAL;
    }

    if (argc >= 3) {
        kc_sha3_update(&context, argv[2], strlen(argv[2]));
    } else {
        bytes_read = 0;
    
        bytes_read = fread(buffer, sizeof(uint8_t), BUFFER_SIZE, stdin);
        while (bytes_read > 0) {
            kc_sha3_update(&context, buffer, bytes_read);
            
            bytes_read = fread(buffer, sizeof(uint8_t), BUFFER_SIZE, stdin);
        }
    }

    kc_sha3_final(&context, digest, &digest_length);

    for (uint32_t i = 0; i < digest_length; i++) {
        printf("%02x", digest[i]);
    }

    kc_sha3_close(&context);

    return 0;
}
#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>


typedef struct {
    EVP_MD const *algorithm;
    EVP_MD_CTX *context;
    uint32_t digest_length_bits;

} HashContext;

void hash_then_print(HashContext context, char const *data, size_t data_length) {
    uint8_t digest[512] = "\0";
    uint32_t digest_length;

    EVP_DigestInit(context.context, context.algorithm);
    EVP_DigestUpdate(context.context, data, data_length);
    EVP_DigestFinal(context.context, digest, &digest_length);

    printf("Sha3-%d(\"%s\") = ", context.digest_length_bits, data);
    for (int i = 0; i < digest_length; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");
}

void Sha3Test() {

    HashContext contexts[4];


    contexts[0].algorithm = EVP_sha3_224();
    contexts[0].context = EVP_MD_CTX_create();
    contexts[0].digest_length_bits = 224;

    contexts[1].algorithm = EVP_sha3_256();
    contexts[1].context = EVP_MD_CTX_create();
    contexts[1].digest_length_bits = 256;

    contexts[2].algorithm = EVP_sha3_384();
    contexts[2].context = EVP_MD_CTX_create();
    contexts[2].digest_length_bits = 384;

    contexts[3].algorithm = EVP_sha3_512();
    contexts[3].context = EVP_MD_CTX_create();
    contexts[3].digest_length_bits = 512;


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
}


int main() {
    Sha3Test();

    return 0;
}
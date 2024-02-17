#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/ketchup_lib.h"

void digest_print(FILE *fp, uint8_t *digest, uint32_t digest_len) {
    if (digest_len == 0) {
        fprintf(fp, "00");
        return;
    }

    for (uint32_t i = 0; i < digest_len; i++) {
        fprintf(fp, "%02x", digest[i]);
    }
}

uint8_t *read_input_line(FILE *fp, uint32_t *buflen ) {
    char byte_buffer[3] = {0};
    uint32_t msg_len = 0;
    int read_result;


    read_result = fscanf(fp, "%d ", &msg_len);

    if (read_result < 1) {
        return NULL;
    }

    uint8_t *output_buffer = malloc(msg_len);

    for (int i = 0; i < msg_len / 8; i++) {
        byte_buffer[0] = fgetc(fp);
        byte_buffer[1] = fgetc(fp);

        sscanf(byte_buffer, "%2x", (uint32_t*) &output_buffer[i]);
    }

    *buflen = msg_len / 8;

    return output_buffer;
}

typedef kc_error kc_sha3_function(const void *, uint32_t, uint8_t*, uint32_t*);

bool write_resp_file(char *infile_path, char *outfile_path) {
    uint8_t *input;
    uint8_t output[KC_MAX_MD_SIZE];
    uint32_t input_len, output_len;
    uint32_t hash_len;
    kc_sha3_function *hash_function;
    FILE *infile, *outfile;

    infile = fopen(infile_path, "r");
    outfile = fopen(outfile_path, "w+");

    if (infile == NULL || outfile == NULL) {
        fprintf(
            stderr,
            "Cannot open files \"%s\" \"%s\"\n",
            infile_path,
            outfile_path
        );
        return false;
    }

    fscanf(infile, "%d", &hash_len);

    switch (hash_len) {
        case 512:
            hash_function = kc_sha3_512;
            break;
        case 384:
            hash_function = kc_sha3_384;
            break;
        case 256:
            hash_function = kc_sha3_256;
            break;
        case 224:
            hash_function = kc_sha3_224;
            break;
        default:
            fprintf(stderr, "Invalid hash size %d\n", hash_len);
            fclose(infile);
            fclose(outfile);
            return false;
    }

    fprintf(outfile, "\n[L = %d]\n\n", hash_len);

    for (;;) {
        input = read_input_line(infile, &input_len);
        if (input == NULL) {
            break;
        }

        hash_function(input, input_len, output, &output_len);

        fprintf(outfile, "Len = %d\nMsg = ", input_len * 8);
        digest_print(outfile, input, input_len);
        fprintf(outfile, "\nMD = ");
        digest_print(outfile, output, output_len);
        fprintf(outfile, "\n\n");

        free(input);
    }

    fclose(infile);
    fclose(outfile);
    return true;
}

bool compute_msg_resp_file(char *resp_name, char *indir, char *outdir) {
    char infile_buff[1024];
    char outfile_buff[1024];

    sprintf(infile_buff, "%s/%s.in", indir, resp_name);
    sprintf(outfile_buff, "%s/%s.rsp", outdir, resp_name);

    return write_resp_file(infile_buff, outfile_buff);
}

void read_md(FILE *fp, uint8_t *bufout, uint32_t buflen) {
    char digits[2];
    for (int i = 0; i < buflen/8; i++) {
        digits[0] = fgetc(fp);
        digits[1] = fgetc(fp);
        sscanf(digits, "%2x", (uint32_t *) &bufout[i]);
    }
}

bool compute_monte_resp_file(char *resp_name, char *indir, char *outdir) {
    char infile_buff[1024];
    char outfile_buff[1024];

    sprintf(infile_buff, "%s/%s.in", indir, resp_name);
    sprintf(outfile_buff, "%s/%s.rsp", outdir, resp_name);

    FILE *infile, *outfile;
    uint32_t hash_len;
    kc_sha3_function *hash_function;

    infile = fopen(infile_buff, "r");
    outfile = fopen(outfile_buff, "w");


    if (infile == NULL || outfile == NULL) {
        fprintf(
            stderr,
            "Cannot open files \"%s\" \"%s\"\n",
            infile_buff,
            outfile_buff
        );
        return false;
    }

    fscanf(infile, "%d ", &hash_len);

    switch (hash_len) {
        case 512:
            hash_function = kc_sha3_512;
            break;
        case 384:
            hash_function = kc_sha3_384;
            break;
        case 256:
            hash_function = kc_sha3_256;
            break;
        case 224:
            hash_function = kc_sha3_224;
            break;
        default:
            fprintf(stderr, "Invalid hash size %d\n", hash_len);
            fclose(infile);
            fclose(outfile);
            return false;
    }

    fprintf(outfile, "\n[L = %d]\n\n", hash_len);
    uint8_t hash[KC_MAX_MD_SIZE], prev_hash[KC_MAX_MD_SIZE];
    uint32_t dummy;

    read_md(infile, prev_hash, hash_len);

    fprintf(outfile, "Seed = ");
    digest_print(outfile, prev_hash, hash_len/8);
    fprintf(outfile, "\n\n");


    for (int j = 0; j < 100; j++) {
        for (int i = 1; i < 1001; i++) {
            hash_function(prev_hash, hash_len/8, hash, &dummy);
            memcpy(prev_hash, hash, hash_len/8);
        }
        fprintf(outfile, "COUNT = %d\nMD = ", j);
        digest_print(outfile, hash, hash_len/8);
        fprintf(outfile, "\n\n");
    }


    fclose(infile);
    fclose(outfile);

    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "USAGE: infolder outfolder");
        return -1;
    }


    printf("Computing SHA3-224...\n");
    compute_msg_resp_file("SHA3_224ShortMsg", argv[1], argv[2]);
    compute_msg_resp_file("SHA3_224LongMsg", argv[1], argv[2]);
    compute_monte_resp_file("SHA3_224Monte", argv[1], argv[2]);

    printf("Computing SHA3-256...\n");
    compute_msg_resp_file("SHA3_256ShortMsg", argv[1], argv[2]);
    compute_msg_resp_file("SHA3_256LongMsg", argv[1], argv[2]);
    compute_monte_resp_file("SHA3_256Monte", argv[1], argv[2]);

    printf("Computing SHA3-384...\n");
    compute_msg_resp_file("SHA3_384ShortMsg", argv[1], argv[2]);
    compute_msg_resp_file("SHA3_384LongMsg", argv[1], argv[2]);
    compute_monte_resp_file("SHA3_384Monte", argv[1], argv[2]);

    printf("Computing SHA3-512...\n");
    compute_msg_resp_file("SHA3_512ShortMsg", argv[1], argv[2]);
    compute_msg_resp_file("SHA3_512LongMsg", argv[1], argv[2]);
    compute_monte_resp_file("SHA3_512Monte", argv[1], argv[2]);

    printf("All done!\n");

}


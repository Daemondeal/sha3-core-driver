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

bool write_resp_file(char *infile_path, char *outfile_path, kc_sha3_function hash_function) {
    uint8_t *input;
    uint8_t output[KC_MAX_MD_SIZE];
    uint32_t input_len, output_len;
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

    fprintf(outfile, "\n[L = 512]\n\n");

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

bool compute_resp_file(char *resp_name, kc_sha3_function hash_function, char *indir, char *outdir) {
    char infile_buff[1024];
    char outfile_buff[1024];

    sprintf(infile_buff, "%s/%s.in", indir, resp_name);
    sprintf(outfile_buff, "%s/%s.rsp", outdir, resp_name);

    return write_resp_file(infile_buff, outfile_buff, hash_function);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "USAGE: infolder outfolder");
        return -1;
    }


    printf("Computing SHA3-224...\n");
    compute_resp_file("SHA3_224ShortMsg", kc_sha3_224, argv[1], argv[2]);
    compute_resp_file("SHA3_224LongMsg", kc_sha3_224, argv[1], argv[2]);

    printf("Computing SHA3-256...\n");
    compute_resp_file("SHA3_256ShortMsg", kc_sha3_256, argv[1], argv[2]);
    compute_resp_file("SHA3_256LongMsg", kc_sha3_256, argv[1], argv[2]);

    printf("Computing SHA3-384...\n");
    compute_resp_file("SHA3_384ShortMsg", kc_sha3_384, argv[1], argv[2]);
    compute_resp_file("SHA3_384LongMsg", kc_sha3_384, argv[1], argv[2]);

    printf("Computing SHA3-512...\n");
    compute_resp_file("SHA3_512ShortMsg", kc_sha3_512, argv[1], argv[2]);
    compute_resp_file("SHA3_512LongMsg", kc_sha3_512, argv[1], argv[2]);

    printf("All done!\n");

}


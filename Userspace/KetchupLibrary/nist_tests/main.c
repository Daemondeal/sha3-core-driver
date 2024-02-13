#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void write_resp_file(FILE *infile, FILE *outfile) {
    uint8_t *input;
    uint8_t output[KC_MAX_MD_SIZE];
    uint32_t input_len, output_len;

    fprintf(outfile, "\n[L = 512]\n\n");

    for (;;) {
        input = read_input_line(infile, &input_len);
        if (input == NULL) {
            return;
        }

        kc_sha3_512(input, input_len, output, &output_len);

        fprintf(outfile, "Len = %d\nMsg = ", input_len * 8);
        digest_print(outfile, input, input_len);
        fprintf(outfile, "\nMD = ");
        digest_print(outfile, output, output_len);
        fprintf(outfile, "\n\n");

        free(input);
    }

}

int main() {
    FILE *infile = fopen("./nist_tests/512_short", "r");
    FILE *outfile = fopen("./nist_tests/out/512_short.resp", "w+");

    if (infile == NULL || outfile == NULL)  {
        fprintf(stderr, "Cannot open files\n");
    }

    write_resp_file(infile, outfile);

    fclose(infile);
    fclose(outfile);
}


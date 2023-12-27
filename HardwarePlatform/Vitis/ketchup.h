#ifndef _KETCHUP_H
#define _KETCHUP_H

#include <xil_types.h>


typedef struct sha3_context_s {
    u32 base_address;
    u32 digest_length;
} Sha3Context;

void sha3_224_init(Sha3Context *context);
void sha3_256_init(Sha3Context *context);
void sha3_384_init(Sha3Context *context);
void sha3_512_init(Sha3Context *context);
void sha3_512_init_2(Sha3Context *context);

void sha3(Sha3Context context, void const *input, u32 input_size, u8 *output, u32 *output_length);

#endif

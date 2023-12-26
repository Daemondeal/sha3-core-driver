#include <stdio.h>
#include <xil_io.h>
#include <xparameters.h>
#include "ketchup.h"

#define KETCHUP_BASE_512_1 XPAR_KETCHUPPERIPHERALPAR_0_BASEADDR
#define KETCHUP_BASE_384_1 XPAR_KETCHUPPERIPHERALPAR_1_BASEADDR
#define KETCHUP_BASE_256_1 XPAR_KETCHUPPERIPHERALPAR_2_BASEADDR
#define KETCHUP_BASE_224_1 XPAR_KETCHUPPERIPHERALPAR_3_BASEADDR
#define KETCHUP_BASE_512_2 XPAR_KETCHUPPERIPHERALPAR_4_BASEADDR


void sha3_512_init_2(Sha3Context *context) {
    context->base_address = KETCHUP_BASE_512_2;
    context->digest_length = 512/8;
}

void sha3_512_init(Sha3Context *context) {
    context->base_address = KETCHUP_BASE_512_1;
    context->digest_length = 512/8;
}

void sha3_384_init(Sha3Context *context) {
    context->base_address = KETCHUP_BASE_384_1;
    context->digest_length = 384/8;
}

void sha3_256_init(Sha3Context *context) {
    context->base_address = KETCHUP_BASE_256_1;
    context->digest_length = 256/8;
}

void sha3_224_init(Sha3Context *context) {
    context->base_address = KETCHUP_BASE_224_1;
    context->digest_length = 224/8;
}

void sha3(Sha3Context context, void const *input, u32 input_size, u8 *output, u32 *output_length) {
    u32 reg_control = context.base_address;
    u32 reg_status = context.base_address + 4;
    u32 reg_input = context.base_address + 8;
    u32 reg_command = context.base_address + 12;
    u32 reg_output = context.base_address + 16;
    printf("Control: %08x\n", reg_control);


    // Reset the peripheral
    Xil_Out32(reg_command, 1);

    // Deal with the 4-byte aligned input
    Xil_Out32(reg_control, 0);
    for (u32 i = 0; i < input_size/4; i++) {
        uint32_t point = ((u32*)input)[i];

        // Reverse Endianness
        uint32_t point_be = ((point << 24) & 0xFF000000) | ((point << 8) & 0xFF0000) | ((point >> 8) & 0xFF00) | ((point >> 24) & 0xFF);

        Xil_Out32(reg_input, point_be);
    }

    u32 remaining = input_size % 4;
    u32 final = 0;
    
    if (remaining > 0) {
        u8 const *last = &input[input_size & ~(0x3)];

        final |= last[0] << 24;
        if (remaining > 1)
            final |= last[1] << 16;
        if (remaining > 2)
            final |= last[2] << 8;
    }
    
    // printf("Last Chunk = %08x (command = %02x)\n", final, (1 << 2) | remaining);
    Xil_Out32(reg_control, (1 << 2) | remaining);
    Xil_Out32(reg_input, final);

    // Now we have to wait for the peripheral to answer
    while ((Xil_In32(reg_status) & 1) == 0);

    for (u32 i = 0; i < context.digest_length; i++) {
        u32 value = Xil_In32(reg_output + 4*i);

        // Unpack values
        output[i*4 + 0] = (value >> 24) & 0xFF;
        output[i*4 + 1] = (value >> 16) & 0xFF;
        output[i*4 + 2] = (value >> 8)  & 0xFF;
        output[i*4 + 3] = (value)       & 0xFF;
    }
    *output_length = context.digest_length;
}

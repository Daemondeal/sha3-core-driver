/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/
/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xiltimer.h"

#define BASE XPAR_KECCAKPERIPHERAL_0_BASEADDR

#define REG_CONTROL (BASE + 0)
#define REG_STATUS  (BASE + 4)
#define REG_INPUT   (BASE + 8)
#define REG_COMMAND (BASE + 12)
#define REG_OUTPUT  (BASE + 16)

void reg_dump() {
    printf("Reg Dump:\n\r");
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 4; j++) {
            int reg_num = i * 4 + j;
            printf("reg%02d: 0x%08x  ", reg_num, Xil_In32(BASE + 4 * reg_num));
        }
        printf("\n\r");
    }
    printf("\n\r");
}

// NOTE: output must be at least 16 u32 long
void keccak(u8 *input, u32 input_size, u32 *output) {
    // First of all, reset the peripheral
    Xil_Out32(REG_COMMAND, 1);

    // We have to send the hash input in packets of four bits


    // Bit 2 clear means "this is not the end of the stream" 
    Xil_Out32(REG_CONTROL, 0);
    // First thing, we deal with the chunks that are four long
    for (u32 i = 0; i < (input_size >> 2); i++) {
        u32 idx = i << 2;

        // Put the chunks together in the correct order
        u32 input_value = input[idx + 3] | input[idx + 2] << 8 | input[idx + 1] << 16 | input[idx] << 24;


        // Set the input register to the current value
        Xil_Out32(REG_INPUT, input_value);
    }

    // We now have to build the last packet
    u32 last_input = 0;
    u32 idx = input_size & ~(3);
    u32 remaining = input_size & 3;
    if (remaining >= 1)
        last_input |= input[idx] << 24;
    if (remaining >= 2)
        last_input |= input[idx + 1] << 16;
    if (remaining >= 3)
        last_input |= input[idx + 2] << 8;
    // Remaining cannot be four


    //                    This is      Remaining Bit 
    //                 the last input     count
    //                       |              |
    //                       v              v
    u32 commit_command = (1 << 2) |    remaining;
    Xil_Out32(REG_CONTROL, commit_command);

    // Send the last input
    Xil_Out32(REG_INPUT, last_input);

    // Now we have to wait for the peripheral to answer
    for (;;) {
        u32 status = Xil_In32(REG_STATUS);
        // Second bit of status is  "Output Ready"
        if ((status & 1) > 0)
            break;
    }


    // Now we can read the output
    // We assume that the out buffer is at least 16 u32 long
    for (u32 i = 0; i < 16; i++) {
        output[i] = Xil_In32(REG_OUTPUT + 4 * i);
    }
}

void print_hash_digest(u32 *hash) {
    for (u32 i = 0; i < 16; i++)
        printf("%08x", hash[i]);
}

void test_case(const char *test_string) {
    u32 hash_buffer[16];
    keccak((u8*)test_string, strlen(test_string), hash_buffer);
    printf("Keccak-512(\"%s\") = ", test_string);
    print_hash_digest(hash_buffer);
    printf("\n\r");
}

void benchmark() {
    u32 hash_buffer[16];
    const u8 test_string[] = "The quick brown fox";
    u32 string_len = strlen((const char*)test_string);

    u32 hash_count = 1000000;

    printf("Hashing %u times the string \"%s\"\n\r", hash_count, test_string);

    printf("Starting now!\n\r");
    // fflush(stdout);
    for (int i = 0; i < 1000000; i++) {
        keccak((u8*)test_string, string_len, hash_buffer);
    }
    printf("Done!\n\r");
    printf("The hash was: ");
    print_hash_digest(hash_buffer);
    printf("\n\r");
}


int main()
{
    init_platform();
    
    // benchmark();

    test_case("AA");
    test_case("The quick brown fox jumps over the lazy dog.");
    test_case("The quick brown fox jumps over the lazy dog. Th");
    test_case("THIS WORKS!");
    test_case("");
    test_case("Banana. Banana. Banana. Banana. Banana. Banana. Banana. Banana. Banana. Banana. Banana. Banana. Banana. Banana.");

    cleanup_platform();
    return 0;
}

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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "xil_printf.h"
#include "ketchup.h"


void hash_then_print(Sha3Context context, char const *input, size_t input_length) {
    u8 digest[512] = {0};
    u32 digest_length;

    sha3(context, input, input_length, digest, &digest_length);

    printf("Sha-%d(\"%s\") = ", context.digest_length * 8, input);
    for (size_t i = 0; i < digest_length; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");
}


int main()
{
    init_platform();

    Sha3Context contexts[4];

    sha3_224_init(&contexts[0]);
    sha3_256_init(&contexts[1]);
    sha3_384_init(&contexts[2]);
    sha3_512_init(&contexts[3]);

    char test1[] = "Hello World";
    char test2[] = "Hello World!";
    char test3[] = "Hello";
    char test4[] = "hi mom";
    char test5[] = "This is a very very very very very very long test test test test test!";

    printf("Beginning Tests:\n\n");
    for (int i = 0; i < 4; i++) {
        Sha3Context context = contexts[i];
        hash_then_print(context, test1, strlen(test1));
        hash_then_print(context, test2, strlen(test2));
        hash_then_print(context, test3, strlen(test3));
        hash_then_print(context, test4, strlen(test4));
        hash_then_print(context, test5, strlen(test5));
        printf("\n");
    }


    cleanup_platform();
    return 0;
}

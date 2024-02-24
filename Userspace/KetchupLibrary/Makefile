# Common sources
SOURCES := ./src/ketchup_lib_common.c
OBJS    := ketchup_lib_common.o

# General Parameters
CC           := gcc
CFLAGS       := -Wall -s -Os #-Werror
LIBS         := 

# Peripheral Parameters
SOURCES_HARDWARE := ./src/ketchup_lib_hardware.c
OBJS_HARDWARE    := ketchup_lib_hardware.o
CFLAGS_HARDWARE  := -D KETCHUP_LIB_MODE=0

# OpenSSL Parameters
LIBS_OPENSSL    := -lcrypto
SOURCES_OPENSSL := ./src/ketchup_lib_openssl.c
OBJS_OPENSSL    := ketchup_lib_openssl.o

# ARM parameters
ARM_CC := arm-linux-gnueabihf-gcc

ARM_INCLUDES_OPENSSL := ./openssl/include
ARM_LIBS_OPENSSL     := ./openssl/libcrypto.a


x64_openssl: $(SOURCES) $(SOURCES_OPENSSL) ./sha3sum/main.c
	$(CC) -o sha3sum.out ./sha3sum/main.c $(SOURCES) $(SOURCES_OPENSSL) $(CFLAGS) $(LIBS) $(LIBS_OPENSSL)
	$(CC) -o nist_tests.out ./nist_tests/main.c $(SOURCES) $(SOURCES_OPENSSL) $(CFLAGS) $(LIBS) $(LIBS_OPENSSL)

arm: $(SOURCES) $(SOURCES_OPENSSL) ./sha3sum/main.c
	$(ARM_CC) -o sha3sum.arm.out ./sha3sum/main.c $(SOURCES) $(SOURCES_HARDWARE) $(CFLAGS) $(CFLAGS_HARDWARE)
	$(ARM_CC) -o nist_tests.arm.out ./nist_tests/main.c $(SOURCES) $(SOURCES_HARDWARE) $(CFLAGS) $(CFLAGS_HARDWARE)

arm_openssl: $(SOURCES) $(SOURCES_OPENSSL) ./sha3sum/main.c
	$(ARM_CC) -o sha3sum.arm_openssl.out ./sha3sum/main.c $(SOURCES) $(SOURCES_OPENSSL) $(CFLAGS) -I$(ARM_INCLUDES_OPENSSL) $(ARM_LIBS_OPENSSL)

.PHONY: clean
clean:
	rm -rf ./*.out ./*.o ./*.a ./*.enc

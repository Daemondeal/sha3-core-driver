# Common sources
SOURCES := ./example.c \
		   ./src/ketchup_lib_common.c

# General Parameters
CC           := gcc
CFLAGS       := -Wall -Werror
LIBS         := 
BINARY_NAME  := example.out

# Peripheral Parameters
SOURCES_HARDWARE := ./src/ketchup_lib_hardware.c
CFLAGS_HARDWARE  := -D KETCHUP_LIB_MODE=0

# OpenSSL Parameters
LIBS_OPENSSL    := -lcrypto
SOURCES_OPENSSL := ./src/ketchup_lib_openssl.c

# ARM parameters
ARM_CC := arm-linux-gnueabihf-gcc

ARM_INCLUDES_OPENSSL := ./openssl/include
ARM_LIBS_OPENSSL     := ./openssl/libcrypto.a


# x86_64 OpenSSL Target
.PHONY: compile
compile: $(BINARY_NAME)

$(BINARY_NAME): $(SOURCES)
	$(CC) $(SOURCES) $(SOURCES_OPENSSL) -o $(BINARY_NAME) $(CFLAGS) $(LIBS) $(LIBS_OPENSSL)


.PHONY: arm
arm: 
	$(ARM_CC) $(SOURCES) $(SOURCES_HARDWARE) -o $(BINARY_NAME).arm $(CFLAGS) $(CFLAGS_HARDWARE)


# ARM OpenSSL Target
.PHONY: arm_openssl
arm_openssl: $(BINARY_NAME).arm_openssl

$(BINARY_NAME).arm_openssl: $(SOURCES) 
	$(ARM_CC) $(SOURCES) $(SOURCES_OPENSSL) -o $(BINARY_NAME).arm_openssl $(CFLAGS) -I$(ARM_INCLUDES_OPENSSL) $(ARM_LIBS_OPENSSL)

.PHONY: clean
clean:
	rm -rf ./*.out
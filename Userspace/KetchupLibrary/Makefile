# Common sources
SOURCES := ./example.c \
		   ./src/ketchup_lib_common.c

# General Parameters
CC           := gcc
CFLAGS       := -Wall -Werror
LIBS         := 
BINARY_NAME  := example.out

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
	@echo '### ERROR: arm target currently not supported'


# ARM OpenSSL Target
.PHONY: arm_openssl
arm_openssl: $(BINARY_NAME).arm_openssl

$(BINARY_NAME).arm_openssl: $(SOURCES) 
	$(ARM_CC) $(SOURCES) $(SOURCES_OPENSSL) -o $(BINARY_NAME).arm $(CFLAGS) -I$(ARM_INCLUDES_OPENSSL) $(ARM_LIBS_OPENSSL)

SOURCES := ./example.c \
		   ./src/ketchup_lib_openssl.c

# General Parameters
CC           := gcc
CFLAGS       := -Wall -Werror
LIBS         := -lcrypto
BINARY_NAME  := example.out


# ARM parameters
ARM_CC := arm-linux-gnueabihf-gcc

ARM_INCLUDES := ./openssl/include
ARM_LIBS     := ./openssl/libcrypto.a


# Regular Target
.PHONY: compile
compile: $(BINARY_NAME)

$(BINARY_NAME): $(SOURCES)
	$(CC) $(SOURCES) -o $(BINARY_NAME) $(CFLAGS) $(LIBS)


.PHONY: arm
arm: 
	@echo '### ERROR: arm target currently not supported'


# ARM OpenSSL Target
.PHONY: arm_openssl
arm_openssl: $(BINARY_NAME).arm_openssl

$(BINARY_NAME).arm_openssl: $(SOURCES) 
	$(ARM_CC) $(SOURCES) -o $(BINARY_NAME).arm $(CFLAGS) -I$(ARM_INCLUDES) $(ARM_LIBS)

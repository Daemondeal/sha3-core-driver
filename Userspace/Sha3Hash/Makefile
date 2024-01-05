SOURCES := ./src/main.c \
		   ../KetchupLibrary/src/ketchup_lib_openssl.c

# General Parameters
CC           := gcc
CFLAGS       := -Wall -Werror
LIBS         := -lcrypto
BINARY_NAME  := sha3hash.out


# ARM parameters
OPENSSL_FOLDER := ../KetchupLibrary/openssl

ARM_CC := arm-linux-gnueabihf-gcc

ARM_INCLUDES := $(OPENSSL_FOLDER)/include
ARM_LIBS     := $(OPENSSL_FOLDER)/libcrypto.a


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

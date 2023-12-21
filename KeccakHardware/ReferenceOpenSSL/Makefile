DEF_CC := gcc
ARM_CC := arm-linux-gnueabihf-gcc
CC := $(ARM_CC)

OPENSSL  := /home/daem/Desktop/Sha3/openssl
INCLUDES := ./include
CFLAGS   := -Wall -Os
BINARY   := sha3test
LIBS     := -lcrypto
LIBS_ARM := ./libcrypto.a
SOURCES  := main.c

all:
	$(DEF_CC) $(SOURCES) $(CFLAGS) -o $(BINARY) $(LIBS)

arm:
	$(ARM_CC) $(SOURCES) -I$(INCLUDES) $(CFLAGS) -o $(BINARY).arm $(LIBS_ARM)	
	uuencode $(BINARY).arm $(BINARY).arm > $(BINARY).enc

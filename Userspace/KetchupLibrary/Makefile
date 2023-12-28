SOURCES := ./example.c ./src/ketchup_lib_openssl.c
CFLAGS := -Wall -Werror
LIBS   := -lcrypto
BINARY_NAME := example.out
CC := gcc


$(BINARY_NAME): $(SOURCES)
	$(CC) $(SOURCES) -o $(BINARY_NAME) $(CFLAGS) $(LIBS)



SOURCES := ./src/main.c \
		   ../KetchupLibrary/src/ketchup_lib_openssl.c

CC           := gcc
CFLAGS       := -Wall -Werror
LIBS         := -lcrypto
BINARY_NAME  := reference.out

.PHONY: compile
compile: $(BINARY_NAME)

$(BINARY_NAME): $(SOURCES)
	$(CC) $(SOURCES) -o $(BINARY_NAME) $(CFLAGS) $(LIBS)

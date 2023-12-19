CC=arm-linux-gnueabihf-gcc

all:
	$(CC) ./main.c -o program
	uuencode ./program program > program.enc

clean:
	rm ./program ./program.enc
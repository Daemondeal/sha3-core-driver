all: main.c
	arm-linux-gnueabihf-gcc main.c -s -Os -o multiproc.out.arm
	uuencode multiproc.out.arm multiproc > multiproc.enc

clean:
	rm -rf ./*.arm ./*.enc
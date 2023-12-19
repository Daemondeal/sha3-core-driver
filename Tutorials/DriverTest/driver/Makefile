CONFIG_MODULE_SIG=n
obj-m += example_driver.o

kernel-version=$(shell uname -r)

all:
	make -C /lib/modules/$(kernel-version)/build M=$(PWD) modules

clean:
	cp ./compile_commands.json ./compile_commands.json.tmp
	make -C /lib/modules/$(kernel-version)/build M=$(PWD) clean
	mv ./compile_commands.json.tmp ./compile_commands.json
CC = gcc
FLAG = -o

obj-m = kernel/keyboard_inc.o

Kernel_Version = $(shell uname -r)

all:
	make -C /lib/modules/$(Kernel_Version)/build M=$(shell pwd) modules
	$(CC) $(FLAG) user user_space/main.c

clean:
	make -C /lib/modules/$(Kernel_Version)/build M=$(shell pwd) clean
	rm user_space/main.c
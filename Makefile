CC = gcc
FLAG = -o
LFLAG = -lX11

obj-m = kernel/keyboard_inc.o
obj-m = kernel/time.o

Kernel_Version = $(shell uname -r)

all:
	make -C /lib/modules/$(Kernel_Version)/build M=$(shell pwd) modules
	$(CC) $(FLAG) user user_space/main.c $(LFLAG)

clean:
	make -C /lib/modules/$(Kernel_Version)/build M=$(shell pwd) clean
	rm user
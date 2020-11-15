obj-m = kernel/keyboard_inc.o

Kernel_Version = $(shell uname -r)

all:
	make -C /lib/modules/$(Kernel_Version)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(Kernel_Version)/build M=$(shell pwd) clean

### How to run

build

```
$ make

```

Kernel modules

```
# Open new card. See log kernfs
$ dmesg -wH

# Loaad the kernel module
$ sudo insmod kernel/keyboard_inc.ko

# Check Major number in logs
# Create the character device with Major: <XXX> and Minor: 0
$ sudo mknod /dev/keyboard_inc c <XXX> 0

```
Run user

```
$ sudo ./user

```
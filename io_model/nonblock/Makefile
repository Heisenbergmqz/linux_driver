export ARCH=arm64
export CROSS_COMPILE=/home/heisenberg/linux/rk3568-linux-6.1/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

obj-m += nonblock.o

KDIR := /home/heisenberg/linux/rk3568-linux-6.1/kernel-6.1
PWD := $(shell pwd)

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
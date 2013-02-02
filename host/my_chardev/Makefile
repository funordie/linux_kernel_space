ifneq ($(KERNELRELEASE),)
obj-m := mychardev.o
else
KDIR := /usr/src/linux-headers-$(shell uname -r)
all:
	$(MAKE) -C $(KDIR) M=`pwd` modules
endif

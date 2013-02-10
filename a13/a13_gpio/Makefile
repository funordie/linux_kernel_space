ARCH=arm
CROSS_COMPILE=arm-linux-gnueabi-
obj-m := a13_gpio.o
KDIR := /home/ipaev/git_repos/linux-3.0-A13/linux-3.0
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

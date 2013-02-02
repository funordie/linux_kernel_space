obj-m := hello_param.o
KDIR := /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KDIR) M=`pwd` modules

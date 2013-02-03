ifneq ($(KERNELRELEASE),)
obj-m := my_v4l2_driver.o
else
KDIR := /usr/src/linux-headers-$(shell uname -r)
all:
	$(MAKE) -C $(KDIR) M=`pwd` modules
clean:
	rm -r  *.ko *.o .*.o.cmd .*.ko.cmd *.mod.c modules.order Module.symvers .tmp_versions
endif

obj-m := a13_lcd_driver.o

KERNEL_DIR=/home/yohoho/android/a13/lichee/linux-3.0
PWD := $(shell pwd)
default:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean

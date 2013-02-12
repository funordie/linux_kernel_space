#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioport.h>		//request_mem_region
#include <asm/io.h>			//ioremap
#include "a13_gpio.h"

#define A13_GPIO_BASE_ADDRESS 0x01C20800
#define A13_GPIO_ADDRESS_SIZE 1024

static dev_t a13_gpio_dev_t = MKDEV(255, 255);

struct cdev a13_cdev;
unsigned int *pc;
static struct class *class_a13_gpio;
static struct device *dev_a13_gpio;

static	ssize_t a13_gpio_open (struct inode *inode, struct file *file)
{
	pr_err("a13_gpio_open\n");
	return 0;
}
static	ssize_t a13_gpio_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	static int pin = 9;

	pr_err("a13_gpio_write buf[0]:%d size:%d offset:%lld\n", buf[0], size , *offset);

	//switch gpio value
	if(size == 2) {
		if(buf[0] == '0') {
			//set pin off
			*(pc+(0xe8>>2)) &= ~(1<<pin);//0x81f;
			pr_err("a13_gpio_write set gpio OFF read value:%#x addr:%#x", *(pc+(0xe8>>2)), ((unsigned int)pc+(0xe8>>2)));
		}
		else if(buf[0] == '1'){
			//set pin on
			*(pc+(0xe8>>2)) |= (1<<pin);//0xa1f;
			pr_err("a13_gpio_write set gpio ON read value:%#x addr:%#x", *(pc+(0xe8>>2)), ((unsigned int)pc+(0xe8>>2)));
		}

		return size;
	}

	return -1;

}
//int (*open) (struct inode *, struct file *);
static	int a13_gpio_release (struct inode *inode, struct file *file)
{
	pr_err("a13_gpio_release\n");
	return 0;
}

static const struct file_operations a13_gpio_fops = {
	.owner = THIS_MODULE,
	.open		= a13_gpio_open,
	.write		= a13_gpio_write,
	.release	= a13_gpio_release
};

static int __init a13_gpio_init(void) {

	void *ptr_err;
	pr_err("a13_gpio_init start\n");

	if (register_chrdev_region(a13_gpio_dev_t, 1, "a13_gpio_device")) {
		pr_err("a13_gpio:Failed to allocate device number\n");
		return -1;
	}

	cdev_init(&a13_cdev, &a13_gpio_fops);

	if (cdev_add(&a13_cdev, a13_gpio_dev_t, 1)) {
		pr_err("a13_gpio: Failed to add device number\n");
		goto error1;
	}

	//mmap MMIO region
	if( request_mem_region(A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE, "a13_gpio_memory") == NULL )
	{
		pr_err("a13_gpio error: unable to obtain I/O memory address:%x\n",A13_GPIO_BASE_ADDRESS);
	}
	else {
		pr_err("a13_gpio I/O memory address:%#x obtain success\n", A13_GPIO_BASE_ADDRESS);
	}
	pc = (u32*)ioremap( A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE);
	if(pc == NULL) {
		pr_err("Unable to remap gpio memory");
		goto error2;
	}
	pr_err("a13_gpio G9 config remap address:%#x\n", (unsigned int)pc);
	//init G9 as output
	*(pc+(0xdc>>2)) = 0x1110;

	//print config value
	pr_err("a13_gpio G9 config value:%#x address:%#x\n", *(pc+(0xdc>>2)), ((unsigned int)pc+(0xdc>>2)));
	pr_err("a13_gpio_init success\n");

	//create device
	class_a13_gpio = class_create(THIS_MODULE, "a13_gpio");
	if (IS_ERR(ptr_err = class_a13_gpio))
		goto error2;

	dev_a13_gpio = device_create(class_a13_gpio, NULL, a13_gpio_dev_t, NULL, "a13_gpio");
	if (IS_ERR(ptr_err = dev_a13_gpio))
		goto error3;

	return 0;

	error3:
		class_destroy(class_a13_gpio);
	error2:
		release_mem_region(A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE);
	error1:
		unregister_chrdev_region(a13_gpio_dev_t, 1);
	return -1;
}

static void __exit a13_gpio_exit(void) {
	pr_err("a13_gpio_exit\n");

	cdev_del(&a13_cdev);
	unregister_chrdev_region(a13_gpio_dev_t, 1);

	release_mem_region(A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE);
	iounmap(pc);

	//remove device
	device_destroy(class_a13_gpio, a13_gpio_dev_t);
	class_destroy(class_a13_gpio);
	return;
}

module_init(a13_gpio_init);
module_exit(a13_gpio_exit);

MODULE_DESCRIPTION("a simple a13_gpio driver");
MODULE_AUTHOR("ilia paev");
MODULE_LICENSE("GPL");

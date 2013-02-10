#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include "a13_gpio.h"

static dev_t a13_gpio_dev_t = MKDEV(255, 255);

static struct cdev a13_cdev;

static	ssize_t a13_gpio_open (struct inode *inode, struct file *file)
{
	pr_err("a13_gpio_open\n");
	return 0;
}
static	ssize_t a13_gpio_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	static int count = 0;
	pr_err("a13_gpio_write buf[0]:%d size:%d offset:%d\n", *buf, size , *offset);
	if(count) return -1;
	count++;
	return 0;
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

	pr_err("a13_gpio_init start\n");

	if (register_chrdev_region(a13_gpio_dev_t, 1, "a13_gpio_device")) {
		pr_err("a13_gpio:Failed to allocate device number\n");
		return -1;
	}

	cdev_init(&a13_cdev, &a13_gpio_fops);

	if (cdev_add(&a13_cdev, a13_gpio_dev_t, 1)) {
		pr_err("a13_gpio: Failed to add device number\n");
		goto unregister;
	}

	pr_err("a13_gpio_init success\n");

	return 0;

	unregister:
		unregister_chrdev_region(a13_gpio_dev_t, 1);
	return -1;
}

static void __exit a13_gpio_exit(void) {
	pr_err("a13_gpio_exit\n");

	cdev_del(&a13_cdev);
	unregister_chrdev_region(a13_gpio_dev_t, 1);

	return;
}

module_init(a13_gpio_init);
module_exit(a13_gpio_exit);

MODULE_DESCRIPTION("a simple a13_gpio driver");
MODULE_AUTHOR("ilia paev");
MODULE_LICENSE("GPL");

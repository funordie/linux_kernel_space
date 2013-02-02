#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include "../include/asm/uaccess.h"

static struct cdev my_cdev;
static dev_t my_dev = MKDEV(202, 128);
static char my_buffer[] = "this is read data from mms driver\n";
static bool flag = 0;

ssize_t my_read (struct file *fp, char __user *buffer, size_t size, loff_t *offset)
{
	ssize_t read_size = 0;
	
	if(flag == 0) {
		if(strlen(my_buffer) < size) {
			read_size = strlen(my_buffer);
			flag = 1;
		}
		copy_to_user(buffer /* to */ , my_buffer /* from */ , strlen(my_buffer));
	}
	else return 0;
	
	printk("file:%s function:%s\n", __FILE__, __FUNCTION__);
	return read_size;
}

ssize_t my_write (struct file *fp, const char __user *buffer, size_t size, loff_t *offset)
{
	printk("file:%s function:%s\n", __FILE__, __FUNCTION__);
	return 0;
}
long my_ioctl (struct file *fp, unsigned int cmd ,unsigned long arg)
{
	switch (cmd) {
		case 0: 
			printk("ioctl command 0 arg:%lu\n", arg);
			break;
		case 1:
			printk("ioctl command 1 arg:%lu\n", arg);
			break;
		default:
			printk("ioctl invalid command!!!!\n");
			return -EFAULT;
			break;
	}
	
	printk("file:%s function:%s\n", __FILE__, __FUNCTION__);
	return 0;
}

/*int (*mmap) (struct file *, struct vm_area_struct *);*/

int my_open (struct inode *i , struct file *fp)
{
	flag = 0;
	printk("file:%s function:%s\n", __FILE__, __FUNCTION__);
	return 0;
}

int my_release (struct inode *i, struct file *fp)
{
	printk("file:%s function:%s\n", __FILE__, __FUNCTION__);
	return 0;
}

static const struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.release = my_release,
	.unlocked_ioctl = my_ioctl,
};

static int __init hello_init(void)
{
	pr_alert("Good morrow");
	pr_alert("to this fair assembly.\n");

	if (register_chrdev_region(my_dev, 1, "mms_device"))
		pr_err("Failed to allocate device number\n");

	cdev_init(&my_cdev, &my_fops);

	if (cdev_add(&my_cdev, my_dev, 1))
		pr_err(KERN_ERR "Char driver registration failed\n");

	return 0;
}
static void __exit hello_exit(void)
{
	pr_alert("Alas, poor world, what treasure");
	pr_alert("hast thou lost!\n");

	cdev_del(&my_cdev);
	unregister_chrdev_region(my_dev, 1);
}


module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MMS module");
MODULE_AUTHOR("Iliya Paev");

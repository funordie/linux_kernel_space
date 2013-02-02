#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/moduleparam.h>

static void *acme_buf;
static int acme_bufsize = 8192;
static int acme_count = 1;
static dev_t acme_dev = MKDEV(202, 128);
static struct cdev acme_cdev;
static ssize_t acme_read(...) {...}
static ssize_t acme_write(...) {...}
static const struct file_operations acme_fops = {
		.owner = THIS_MODULE,
		.read = acme_read,
		.write = acme_write,
};

static int __init acme_init(void)
{
	int err;
	acme_buf = ioremap(ACME_PHYS, acme_bufsize);
	if (!acme_buf) {
		err = -ENOMEM;
		goto err_exit;
	}
	if (register_chrdev_region(acme_dev, acme_count, "acme")) {
		err = -ENODEV;
		goto err_free_buf;
	}
	cdev_init(&acme_cdev, &acme_fops);
	if (cdev_add(&acme_cdev, acme_dev, acme_count)) {
		err = -ENODEV;
		goto err_dev_unregister;
	}
	return 0;
	err_dev_unregister:
	unregister_chrdev_region(acme_dev, acme_count);
	err_free_buf:
	iounmap(acme_buf);
	err_exit:
	return err;
}

static void __exit acme_exit(void)
{
	cdev_del(&acme_cdev);
	unregister_chrdev_region(acme_dev, acme_count);
	iounmap(acme_buf);
}
module_init(acme_init);
module_exit(acme_exit);

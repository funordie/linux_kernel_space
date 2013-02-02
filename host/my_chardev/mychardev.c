#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>


#define MYCHAR_MAJOR 0

int mychardev_major = MYCHAR_MAJOR;

/* driver parameters */
static char *whom = "world";
static int mychar_nr_devs = 1;

module_param(mychar_nr_devs, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);
MODULE_AUTHOR("Iliya Paev");
MODULE_LICENSE("Dual BSD/GPL");

/* mychardev structure structure */
struct mychar_dev {
	struct cdev cdev;
};

struct mychar_dev *mychar_devices; /* allocated in driver _init function*/

static ssize_t mychar_fops_read(struct file * file, char __user * user, size_t size, loff_t * count)
{
	return 0;
}

static ssize_t mychar_fops_write (struct file * file, const char __user * user, size_t size, loff_t *count)
{
	return 0;
}

static long mychar_fops_unlocked_ioctl (struct file * file, unsigned int cmd_in, unsigned long arg)
{
	return 0;
}

static int mychar_fops_mmap (struct file * file, struct vm_area_struct * area)
{
	return 0;
}
static int mychar_fops_open (struct inode * inode, struct file * file)
{
	return 0;
}

static int mychar_fops_flush (struct file * file, fl_owner_t id)
{
	return 0;
}

struct file_operations mychar_fops = {
	.owner = THIS_MODULE,
	.read = mychar_fops_read,
	.write = mychar_fops_write,
	.unlocked_ioctl = mychar_fops_unlocked_ioctl,
	.open = mychar_fops_open,
	.mmap = mychar_fops_mmap,
	.open = mychar_fops_open,
	.flush = mychar_fops_flush,
};

static void mychardev_setup_cdev(struct mychar_dev *dev, int index)
{
	int err, devno;


	cdev_init(&dev->cdev, &mychar_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &mychar_fops;

	devno = MKDEV(255, 25);
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
		printk(KERN_INFO "Error: adding mychardev module");
	else
		printk(KERN_INFO "Success: adding mychardev module");

}
/* init and exit functions */
static int __init mychardev_init(void)
{
	int i, result;
	dev_t my_dev_t;

	printk(KERN_INFO "Loading mychardev module...\n");

	my_dev_t = MKDEV(mychardev_major, 0);

	/*
	 * Register your major, and accept a dynamic number.
	 */
	if (mychardev_major)
		result = register_chrdev_region(my_dev_t, mychar_nr_devs, "mychardev");
	else {
		result = alloc_chrdev_region(&my_dev_t, 0, mychar_nr_devs, "mychardev");
		mychardev_major = MAJOR(my_dev_t);
	}
	if (result < 0)
		return result;

	/*
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	mychar_devices = kmalloc(mychar_nr_devs * sizeof(struct mychar_dev), GFP_KERNEL);
	if (!mychar_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(mychar_devices, 0, mychar_nr_devs * sizeof(struct mychar_dev));

	mychardev_setup_cdev(mychar_devices, 1);

	for(i=0;i<mychar_nr_devs;i++)
		printk(KERN_INFO "Hello %s\n", whom);
	return 0;

	fail:
	unregister_chrdev_region(my_dev_t, mychar_nr_devs);
	return result;
}
static void __exit mychardev_exit(void)
{
	int i;

	unregister_chrdev_region(MKDEV (mychardev_major, 0), mychar_nr_devs);

	for (i = 0; i < mychar_nr_devs; i++) {
		cdev_del(&mychar_devices[i].cdev);
		//scullc_trim(scullc_devices + i);
	}
	kfree(mychar_devices);

	for(i=0;i<mychar_nr_devs;i++)
		printk(KERN_INFO "Goodbye Mr.%s\n",whom);
}
module_init(mychardev_init);
module_exit(mychardev_exit);

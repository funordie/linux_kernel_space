#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/types.h>
#include <asm/uaccess.h>

#define MYCHAR_MAJOR 0

int mychardev_major = MYCHAR_MAJOR;

// debugging macros so we can pin down message origin at a glance
#define FLE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#define DEBUGPRINT(LEVEL, _fmt, ...)  printk(LEVEL "[file:%s, line:%d]: " _fmt"\n", FLE, __LINE__ , ##__VA_ARGS__)
//__VA_ARGS__
//#define print(LEVEL, MSG, ARG) printk(LEVEL "[FILE:%s LINE:%d]:"MSG), __FILE__, __LINE__, ARG);

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
	bool bReadData;
};

struct mychar_dev *mychar_devices; /* allocated in driver _init function*/

static ssize_t mychar_fops_read(struct file * filp, char __user * buf, size_t count, loff_t * f_pos)
{
	static char dummy_data[] = "read_data_string\n";
	ssize_t retval = 0;
	struct mychar_dev *dev = filp->private_data; /* the first listitem */

	DEBUGPRINT(KERN_INFO,"mychar_fops_read start");

	if(dev->bReadData) {
		dev->bReadData = false;
	}
	else {
		DEBUGPRINT(KERN_INFO,"mychar_fops_read return 0");
		return 0;
	}
	if (*f_pos > sizeof(dummy_data)) {
		DEBUGPRINT(KERN_INFO,"mychar_fops_read return -1");
		return -1;
	}
	if (copy_to_user(buf,dummy_data, sizeof(dummy_data))) {
		retval = -EFAULT;
		goto nothing;
	}

	*f_pos += count;

	if(count > sizeof(dummy_data)) {
		DEBUGPRINT(KERN_INFO,"mychar_fops_read return sizeof(dummy_data):%d", sizeof(dummy_data));
		return sizeof(dummy_data);
	}

	DEBUGPRINT(KERN_INFO,"mychar_fops_read return count:%d",count);
	return count;

	nothing:
	DEBUGPRINT(KERN_INFO,"mychar_fops_read return retval:%d",retval);
	return retval;
	return 0;
}

static ssize_t mychar_fops_write (struct file * filp, const char __user * user, size_t size, loff_t * f_pos)
{
	printk(KERN_INFO "mychar_fops_write");
	return 0;
}

static long mychar_fops_unlocked_ioctl (struct file * filp, unsigned int cmd_in, unsigned long arg)
{
	printk(KERN_INFO "mychar_fops_unlocked_ioctl");
	return 0;
}

static int mychar_fops_mmap (struct file * filp, struct vm_area_struct * area)
{
	printk(KERN_INFO "mychar_fops_mmap");
	return 0;
}
static int mychar_fops_open (struct inode * inode, struct file * filp)
{
	struct mychar_dev *dev; /* device information */

	printk(KERN_INFO "mychar_fops_open");

	/*  Find the device */
	dev = container_of(inode->i_cdev, struct mychar_dev, cdev);

	//mark data to be read
	dev->bReadData = true;

	/* and use filp->private_data to point to the device data */
	filp->private_data = dev;
	return 0;
}

struct file_operations mychar_fops = {
	.owner = THIS_MODULE,
	.read = mychar_fops_read,
	.write = mychar_fops_write,
	.unlocked_ioctl = mychar_fops_unlocked_ioctl,
	.open = mychar_fops_open,
	.mmap = mychar_fops_mmap,
};

static void mychardev_setup_cdev(struct mychar_dev *dev, int index)
{
	int err, devno;


	cdev_init(&dev->cdev, &mychar_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &mychar_fops;

	devno = MKDEV(mychardev_major, index);
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
		printk(KERN_INFO "Error: adding mychardev module maj:%d min:%d", MAJOR(devno), MINOR(devno));
	else
		printk(KERN_INFO "Success: adding mychardev module maj:%d min:%d", MAJOR(devno), MINOR(devno));
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

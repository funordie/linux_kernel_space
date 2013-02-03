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

#include <linux/string.h>
/* v2l2 headers*/
#include <linux/videodev2.h>

#define MYV4L_MAJOR 81

int myv4ldev_major = MYV4L_MAJOR;

// debugging macros so we can pin down message origin at a glance
#define FLE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#define DEBUGPRINT(LEVEL, _fmt, ...)  printk(LEVEL "[file:%s, line:%d]: " _fmt"\n", FLE, __LINE__ , ##__VA_ARGS__)
//__VA_ARGS__
//#define print(LEVEL, MSG, ARG) printk(LEVEL "[FILE:%s LINE:%d]:"MSG), __FILE__, __LINE__, ARG);

/* driver parameters */
static char *whom = "world";
static int myv4l_nr_devs = 1;

module_param(myv4l_nr_devs, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);
MODULE_AUTHOR("Iliya Paev");
MODULE_LICENSE("Dual BSD/GPL");

/* mychardev structure structure */
struct myv4l_dev {
	struct cdev cdev;

	/* v4l2 driver parameters */
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_streamparm streamparm;

	bool bReadData;	/*tempory variamle used for dummy_data when read file operations*/
};

struct myv4l_dev *myv4l_devices; /* allocated in driver _init function*/

static ssize_t myv4l_fops_read(struct file * filp, char __user * buf, size_t count, loff_t * f_pos)
{
	static char dummy_data[] = "read_data_string\n";
	ssize_t retval = 0;
	struct myv4l_dev *dev = filp->private_data; /* the first listitem */

	DEBUGPRINT(KERN_INFO,"myv4l_fops_read start");

	if(dev->bReadData) {
		dev->bReadData = false;
	}
	else {
		DEBUGPRINT(KERN_INFO,"myv4l_fops_read return 0");
		return 0;
	}
	if (*f_pos > sizeof(dummy_data)) {
		DEBUGPRINT(KERN_INFO,"myv4l_fops_read return -1");
		return -1;
	}
	if (copy_to_user(buf,dummy_data, sizeof(dummy_data))) {
		retval = -EFAULT;
		goto nothing;
	}

	*f_pos += count;

	if(count > sizeof(dummy_data)) {
		DEBUGPRINT(KERN_INFO,"myv4l_fops_read return sizeof(dummy_data):%d", sizeof(dummy_data));
		return sizeof(dummy_data);
	}

	DEBUGPRINT(KERN_INFO,"myv4l_fops_read return count:%d",count);
	return count;

	nothing:
	DEBUGPRINT(KERN_INFO,"myv4l_fops_read return retval:%d",retval);
	return retval;
	return 0;
}

static ssize_t myv4l_fops_write (struct file * filp, const char __user * user, size_t size, loff_t * f_pos)
{
	printk(KERN_INFO "myv4l_fops_write");
	return 0;
}

static long myv4l_fops_unlocked_ioctl (struct file * filp, unsigned int cmd_in, unsigned long arg)
{
	struct v4l2_capability *cap;
	struct v4l2_format *fmt;
	struct v4l2_requestbuffers *req;
	struct v4l2_streamparm *streamparm;
	struct v4l2_buffer *buf;

	struct myv4l_dev *dev = filp->private_data; /* the first listitem */

	//printk(KERN_INFO "myv4l_fops_unlocked_ioctl");
	DEBUGPRINT(KERN_INFO,"myv4l_fops_unlocked_ioctl cmd_in:%d", cmd_in);
	switch(cmd_in) {
	case VIDIOC_QUERYCAP :
		cap = (struct v4l2_capability*)arg;
		cap->capabilities = dev->cap.capabilities;
		DEBUGPRINT(KERN_INFO,"myv4l_fops_unlocked_ioctl VIDIOC_QUERYCAP return:0x%x", cap->capabilities);
		break;

	case VIDIOC_S_FMT :
		fmt = (struct v4l2_format*)arg;
		/*TODO: check for supported parameter*/
		memcpy(&dev->fmt, fmt, sizeof(struct v4l2_format));
		DEBUGPRINT(KERN_INFO,"myv4l_fops_unlocked_ioctl VIDIOC_S_FMT");
		break;

	case VIDIOC_G_FMT :
		fmt = (struct v4l2_format*)arg;
		memcpy(fmt, &dev->fmt, sizeof(struct v4l2_format));
		DEBUGPRINT(KERN_INFO,"myv4l_fops_unlocked_ioctl VIDIOC_G_FMT");
		break;

	case VIDIOC_S_PARM :
		streamparm = (struct v4l2_streamparm*)arg;
		/*TODO: check for supported parameter*/
		memcpy(&dev->streamparm, streamparm, sizeof(struct v4l2_streamparm));
		DEBUGPRINT(KERN_INFO,"myv4l_fops_unlocked_ioctl VIDIOC_G_FMT");
		break;

	case VIDIOC_REQBUFS :
		req = (struct v4l2_requestbuffers*)arg;
		DEBUGPRINT(KERN_INFO,"myv4l_fops_unlocked_ioctl VIDIOC_REQBUFS");
		//req->
		break;

	case VIDIOC_QUERYBUF :
		buf = (struct v4l2_buffer*)arg;

		break;
	}
	return 0;
}

static int myv4l_fops_mmap (struct file * filp, struct vm_area_struct * area)
{
	printk(KERN_INFO "myv4l_fops_mmap");
	return 0;
}
static int myv4l_fops_open (struct inode * inode, struct file * filp)
{
	struct myv4l_dev *dev; /* device information */

	printk(KERN_INFO "myv4l_fops_open");

	/*  Find the device */
	dev = container_of(inode->i_cdev, struct myv4l_dev, cdev);

	//mark data to be read
	dev->bReadData = true;

	/* and use filp->private_data to point to the device data */
	filp->private_data = dev;
	return 0;
}

struct file_operations myv4l_fops = {
	.owner = THIS_MODULE,
	.read = myv4l_fops_read,
	.write = myv4l_fops_write,
	.unlocked_ioctl = myv4l_fops_unlocked_ioctl,
	.open = myv4l_fops_open,
	.mmap = myv4l_fops_mmap,
};

static void myv4ldev_setup_cdev(struct myv4l_dev *dev, int index)
{
	int err, devno;


	cdev_init(&dev->cdev, &myv4l_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &myv4l_fops;

	devno = MKDEV(myv4ldev_major, index);
	err = cdev_add(&dev->cdev, devno, 1);

	//init device structure
	dev->cap.capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

	if(err)
		printk(KERN_INFO "Error: adding myv4ldev module maj:%d min:%d", MAJOR(devno), MINOR(devno));
	else
		printk(KERN_INFO "Success: adding myv4ldev module maj:%d min:%d", MAJOR(devno), MINOR(devno));
}
/* init and exit functions */
static int __init myv4ldev_init(void)
{
	int i, result;
	dev_t my_dev_t;

	printk(KERN_INFO "Loading myv4ldev module...\n");

	my_dev_t = MKDEV(myv4ldev_major, 0);

	/*
	 * Register your major, and accept a dynamic number.
	 */
	if (myv4ldev_major)
		result = register_chrdev_region(my_dev_t, myv4l_nr_devs, "myv4ldev");
	else {
		result = alloc_chrdev_region(&my_dev_t, 0, myv4l_nr_devs, "myv4ldev");
		myv4ldev_major = MAJOR(my_dev_t);
	}
	if (result < 0)
		return result;

	/*
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	myv4l_devices = kmalloc(myv4l_nr_devs * sizeof(struct myv4l_dev), GFP_KERNEL);
	if (!myv4l_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(myv4l_devices, 0, myv4l_nr_devs * sizeof(struct myv4l_dev));

	for(i=0;i<myv4l_nr_devs;i++)
		myv4ldev_setup_cdev(myv4l_devices, i+1);

	for(i=0;i<myv4l_nr_devs;i++)
		printk(KERN_INFO "Hello %s\n", whom);
	return 0;

	fail:
	unregister_chrdev_region(my_dev_t, myv4l_nr_devs);
	return result;
}
static void __exit myv4ldev_exit(void)
{
	int i;

	unregister_chrdev_region(MKDEV (myv4ldev_major, 0), myv4l_nr_devs);

	for (i = 0; i < myv4l_nr_devs; i++) {
		cdev_del(&myv4l_devices[i].cdev);
		//scullc_trim(scullc_devices + i);
	}
	kfree(myv4l_devices);

	for(i=0;i<myv4l_nr_devs;i++)
		printk(KERN_INFO "Goodbye Mr.%s\n",whom);
}
module_init(myv4ldev_init);
module_exit(myv4ldev_exit);

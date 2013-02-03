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

/* v4l2 headers*/
#include <linux/videodev2.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>


#define MYV4L_MODULE_NAME "myv4l"

/* debugging macros so we can pin down message origin at a glance*/
#define FLE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#define DEBUGPRINT(LEVEL, _fmt, ...)  printk(LEVEL "[file:%s, line:%d]: " _fmt"\n", FLE, __LINE__ , ##__VA_ARGS__)

/* driver parameters */
static int myv4l_nr_devs = 1;
module_param(myv4l_nr_devs, int, S_IRUGO);

MODULE_AUTHOR("Iliya Paev");
MODULE_LICENSE("Dual BSD/GPL");

/* mychardev structure structure */
struct myv4l_dev {
	struct cdev cdev;
	struct v4l2_device 	   v4l2_dev;

	struct video_device        *vfd;
};

/* ------------------------------------------------------------------
	IOCTL vidioc handling
   ------------------------------------------------------------------*/
static int vidioc_querycap(struct file *file, void  *priv, struct v4l2_capability *cap)
{
	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv, struct v4l2_fmtdesc *f)
{
	return 0;
}

static int vidioc_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	return 0;
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	return 0;
}

static int vidioc_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	return 0;
}

static int vidioc_reqbufs(struct file *file, void *priv, struct v4l2_requestbuffers *p)
{
	return 0;
}

static int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return 0;
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return 0;
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return 0;
}

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	return 0;
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	return 0;
}

static int vidioc_s_std(struct file *file, void *priv, v4l2_std_id *i)
{
	return 0;
}

/* only one input in this sample driver */
static int vidioc_enum_input(struct file *file, void *priv, struct v4l2_input *inp)
{
	return 0;
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	return 0;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	return 0;
}

static const struct v4l2_ioctl_ops myv4l_ioctl_ops = {
	.vidioc_querycap      = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap  = vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = vidioc_s_fmt_vid_cap,
	.vidioc_reqbufs       = vidioc_reqbufs,
	.vidioc_querybuf      = vidioc_querybuf,
	.vidioc_qbuf          = vidioc_qbuf,
	.vidioc_dqbuf         = vidioc_dqbuf,
	.vidioc_s_std         = vidioc_s_std,
	.vidioc_enum_input    = vidioc_enum_input,
	.vidioc_g_input       = vidioc_g_input,
	.vidioc_s_input       = vidioc_s_input,
	.vidioc_streamon      = vidioc_streamon,
	.vidioc_streamoff     = vidioc_streamoff,
};

/* v4l2_file_operations operations */
static int myv4l_fops_close(struct file *file)
{
	DEBUGPRINT(KERN_INFO,"myv4l_fops_close");
	return 0;
}

static ssize_t myv4l_fops_read(struct file * filp, char __user * buf, size_t count, loff_t * f_pos)
{
	DEBUGPRINT(KERN_INFO,"myv4l_fops_read");
	return 0;
}

static int myv4l_fops_mmap (struct file * filp, struct vm_area_struct * area)
{
	DEBUGPRINT(KERN_INFO,"v4l2_fh_open");
	return 0;
}

static const struct v4l2_file_operations myv4l_fops = {
	.owner		= THIS_MODULE,
	.open		= v4l2_fh_open,
	.release        = myv4l_fops_close,
	.read           = myv4l_fops_read,
	.unlocked_ioctl = video_ioctl2, /* V4L2 ioctl handler */
	.mmap           = myv4l_fops_mmap,
};

static struct video_device myv4l_template = {
	.name		= "myv4l",
	.fops           = &myv4l_fops,
	.ioctl_ops 	= &myv4l_ioctl_ops,
	.release	= video_device_release,

	.tvnorms              = V4L2_STD_525_60,
	.current_norm         = V4L2_STD_NTSC_M,
};

/* Init driver */
static int __init myv4l_create_instance(int inst)
{
	struct myv4l_dev *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
		if (!dev)
			return -ENOMEM;

	snprintf(dev->v4l2_dev.name, sizeof(dev->v4l2_dev.name),
			"%s-%03d", MYV4L_MODULE_NAME, inst);
	ret = v4l2_device_register(NULL, &dev->v4l2_dev);

	return 0;
}
/* init and exit functions */
static int __init myv4ldev_init(void)
{
	int ret = 0, i;

	if (myv4l_nr_devs <= 0)
		myv4l_nr_devs = 1;

	for (i = 0; i < myv4l_nr_devs; i++) {
		ret = myv4l_create_instance(i);
		if (ret) {
			/* If some instantiations succeeded, keep driver */
			if (i)
				ret = 0;
			break;
		}
	}

	if (ret < 0) {
		printk(KERN_ERR "myv4l: error %d while loading driver\n", ret);
		return ret;
	}

	printk(KERN_INFO "myv4ldev_init success");

	/* n_devs will reflect the actual number of allocated devices */
	myv4l_nr_devs = i;

	return ret;
}
static void __exit myv4ldev_exit(void)
{
	return;
}
module_init(myv4ldev_init);
module_exit(myv4ldev_exit);

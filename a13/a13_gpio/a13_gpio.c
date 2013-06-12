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

#include <asm-generic/uaccess.h> //copy_to_user; copy_from_user

#include <linux/vmalloc.h>

#define A13_GPIO_BASE_ADDRESS 0x01C20800
#define A13_GPIO_ADDRESS_SIZE 1024

#define __FILE_NAME__ strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__

#define PRINT(_fmt, ...)  \
	printk("[file:%s, line:%d]: " _fmt"\n", __FILE_NAME__, __LINE__ , ##__VA_ARGS__)


struct a13_dev_struct {
	struct cdev a13_cdev_value;
	struct cdev a13_cdev_config;
};

struct a13_dev_struct a13_dev;

//value device
static dev_t a13_gpio_value_dev_t = MKDEV(255, 255);
//config device
static dev_t a13_gpio_config_dev_t = MKDEV(255, 254);

static struct class *class_a13_gpio_config;
static struct class *class_a13_gpio_value;

static struct device *dev_a13_gpio_config;
static struct device *dev_a13_gpio_value;

struct sunxi_gpio_reg *sunxi_gpio;

static unsigned long ulReadCount = 0;

/*////////////////////////////////////////////  file_operations - value ////////////////////////////////////////////*/
static	ssize_t a13_gpio_value_open (struct inode *inode, struct file *file)
{
	pr_err("a13_gpio_value_open\n");
	ulReadCount = 0;
	return 0;
}
static loff_t a13_gpio_value_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t newpos;

    switch(whence) {
      case 0: /* SEEK_SET */
        newpos = offset;
        break;

      case 1: /* SEEK_CUR */
        newpos = file->f_pos + offset;
        break;

      case 2: /* SEEK_END */
    	return -EINVAL;
        break;

      default: /* can't happen */
        return -EINVAL;
    }
    if (newpos < 0) return -EINVAL;
    file->f_pos = newpos;
    return newpos;
	return 0;
}

static ssize_t a13_gpio_value_read(struct file *file, char __user *buf, size_t size, loff_t *offset) {
	unsigned long  port;
	struct sunxi_gpio * gpio_bank_pointer;
	char ch[10] = "test\n";

	pr_err("a13_gpio_value_read size:%d offset:%lld file->f_pos:%d\n", size , *offset, file->f_pos);

	if(ulReadCount != 0) return 0;

	ulReadCount++;

	//set port
	port = *offset >> 2;

	//get selected bank pointer
	gpio_bank_pointer = &sunxi_gpio->gpio_bank[port];

	copy_to_user(buf, &gpio_bank_pointer->dat, sizeof(gpio_bank_pointer->dat));

	pr_err("a13_gpio_value_read result address:0x%p value:%#x size:%d\n", &gpio_bank_pointer->dat , gpio_bank_pointer->dat, sizeof(gpio_bank_pointer->dat));

	return sizeof(gpio_bank_pointer->dat);
}
static	ssize_t a13_gpio_value_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	unsigned long pin, port;
	struct sunxi_gpio * gpio_bank_pointer;
	char ch[3];

    char *read_buffer = vmalloc(size * sizeof(*read_buffer));
    if (read_buffer == NULL)
        return -ENOMEM;
    if (copy_from_user(read_buffer, buf, size))
    {
        vfree(read_buffer);
        return -EFAULT;
    }
    /* process buffer */


	pr_err("a13_gpio_value_write buf[0]:%d size:%d offset:%lld\n", read_buffer[0], size , *offset);

	//set pin value
	//input data format: A BB C
	// A - port number [A - I]
	// BB - pin number [00 - 31]
	// C - value [0-off 1-on]
	//example: G 09 1 - G9 pin command:ON

	if(size == 7 || size == 6) {
		switch(read_buffer[0]) {
		case 'B':
			//selected port - B
			port = 1;
			break;
		case 'E':
			//selected port - E
			port = 4;
			break;
		case 'G':
			//selected port - G
			port = 6;
			break;
		case 'F':
			//selected port - F
			port = 7;
			break;
		default :
			return -1;
		}

		gpio_bank_pointer = &sunxi_gpio->gpio_bank[port];

		ch[0] = read_buffer[2];
		ch[1] = read_buffer[3];
		ch[2] = 0;
		kstrtol(ch, 10, &pin);

		pr_err("a13_gpio_value_write string:%d, %d, %d,%d, %d, %d, %d, %d"
				, read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3], read_buffer[4], read_buffer[5], read_buffer[6], read_buffer[7] , read_buffer[8]);

		pr_err("a13_gpio_value_write port:%lu pin:%lu value:%c\n", port, pin, read_buffer[5]);

		pr_err("a13_gpio_value_write before write address:%p value:%#018x\n", &gpio_bank_pointer->dat, gpio_bank_pointer->dat);

		if(read_buffer[5] == '1') {
			//set ON
			gpio_bank_pointer->dat |= (1 << pin);
		}
		else if(read_buffer[5] == '0') {
			//set OFF
			gpio_bank_pointer->dat &= ~(1 << pin);
		}
		pr_err("a13_gpio_value_write after write address:%p value:%#018x\n", &gpio_bank_pointer->dat, gpio_bank_pointer->dat);

		vfree(read_buffer);
	    return size;
	}

	vfree(read_buffer);
	return -1;

}
//int (*open) (struct inode *, struct file *);
static	int a13_gpio_value_release (struct inode *inode, struct file *file)
{
	pr_err("a13_gpio_value_release\n");
	return 0;
}

static const struct file_operations a13_gpio_value_fops = {
	.owner = THIS_MODULE,
	.open		= a13_gpio_value_open,
	.read 		= a13_gpio_value_read,
	.llseek		= a13_gpio_value_llseek,
	.write		= a13_gpio_value_write,
	.release	= a13_gpio_value_release
};

/*////////////////////////////////////////////  file_operations - config ////////////////////////////////////////////*/
static	ssize_t a13_gpio_config_open (struct inode *inode, struct file *file)
{
	pr_err("a13_gpio_config_open\n");
	return 0;
}
static	ssize_t a13_gpio_config_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	unsigned long pin, port;
	struct sunxi_gpio * gpio_bank_pointer;
	char ch[3];

	pr_err("a13_gpio_config_write buf[0]:%d size:%d offset:%lld\n", buf[0], size , *offset);

	//set pin configurations
	//input data format: A BB C
	// A - port number [A - I]
	// BB - pin number [00 - 31]
	// C - direction [0-input 1-output]
	//example: G 09 0 - G9 direction output

	if(size == 7) {
		switch(buf[0]) {
		case 'B':
			//selected port - B
			port = 1;
			break;
		case 'E':
			//selected port - E
			port = 4;
			break;
		case 'G':
			//selected port - G
			port = 6;
			break;
		case 'F':
			//selected port - F
			port = 7;
			break;
		default :
			return -1;
		}

		gpio_bank_pointer = &sunxi_gpio->gpio_bank[port];

		ch[0] = buf[2];
		ch[1] = buf[3];
		ch[2] = 0;
		kstrtol(ch, 10, &pin);

		pr_err("a13_gpio_config_write port:%lu pin:%lu\n", port, pin);

		pr_err("a13_gpio_config_write before write address:%p value:%#018x\n", &gpio_bank_pointer->cfg[pin >> 3], gpio_bank_pointer->cfg[pin >> 3]);

		if(buf[5] == '1') {
			//set direction output
			gpio_bank_pointer->cfg[pin >> 3] |= (1 << ((pin-((pin>>3)*8))*4));
		}
		else if(buf[5] == '0') {
			//set direction input
			gpio_bank_pointer->cfg[pin >> 3] &= ~(1 << ((pin-((pin>>3)*8))*4));
		}
		pr_err("a13_gpio_config_write after write address:%p value:%#018x\n", &gpio_bank_pointer->cfg[pin >> 3], gpio_bank_pointer->cfg[pin >> 3]);
		return size;
	}

	return -1;

}
//int (*open) (struct inode *, struct file *);
static	int a13_gpio_config_release (struct inode *inode, struct file *file)
{
	pr_err("a13_gpio_config_release\n");
	return 0;
}

static const struct file_operations a13_gpio_config_fops = {
	.owner = THIS_MODULE,
	.open		= a13_gpio_config_open,
	.write		= a13_gpio_config_write,
	.release	= a13_gpio_config_release
};

static void deregister_and_delete_devices (void) {

	//delete devices
	device_destroy(class_a13_gpio_value, a13_gpio_value_dev_t);
	class_destroy(class_a13_gpio_value);
	device_destroy(class_a13_gpio_config, a13_gpio_config_dev_t);
	class_destroy(class_a13_gpio_config);

	//deregister devices
	cdev_del(&a13_dev.a13_cdev_value);
	unregister_chrdev_region(a13_gpio_value_dev_t, 1);
	cdev_del(&a13_dev.a13_cdev_config);
	unregister_chrdev_region(a13_gpio_config_dev_t, 1);
}

static void register_and_create_devices (void) {

	void *ptr_err;

	//register config device
	if (register_chrdev_region(a13_gpio_config_dev_t, 1, "a13_gpio_config_device")) {
		pr_err("a13_gpio:Failed to allocate config device number\n");
		return;
	}
	cdev_init(&a13_dev.a13_cdev_config, &a13_gpio_config_fops);
	if (cdev_add(&a13_dev.a13_cdev_config, a13_gpio_config_dev_t, 1)) {
		pr_err("a13_gpio: Failed to add config device number\n");
		goto error1;
	}

	//register value device
	if (register_chrdev_region(a13_gpio_value_dev_t, 1, "a13_gpio_value_device")) {
		pr_err("a13_gpio:Failed to allocate value device number\n");
		goto error2;
	}
	cdev_init(&a13_dev.a13_cdev_value, &a13_gpio_value_fops);
	if (cdev_add(&a13_dev.a13_cdev_value, a13_gpio_value_dev_t, 1)) {
		pr_err("a13_gpio: Failed to add value device number\n");
		goto error3;
	}

	//create config device
	class_a13_gpio_config = class_create(THIS_MODULE, "a13_gpio_config");
	if (IS_ERR(ptr_err = class_a13_gpio_config))
		goto error4;
	dev_a13_gpio_config = device_create(class_a13_gpio_config, NULL, a13_gpio_config_dev_t, NULL, "a13_gpio_config");
	if (IS_ERR(ptr_err = dev_a13_gpio_config))
		goto error5;

	//create value device
	class_a13_gpio_value = class_create(THIS_MODULE, "a13_gpio_value");
	if (IS_ERR(ptr_err = class_a13_gpio_value))
		goto error6;
	dev_a13_gpio_value = device_create(class_a13_gpio_value, NULL, a13_gpio_value_dev_t, NULL, "a13_gpio_value");
	if (IS_ERR(ptr_err = dev_a13_gpio_value))
		goto error7;

	return;

	error7:
		class_destroy(class_a13_gpio_value);
	error6:
		device_destroy(class_a13_gpio_value, a13_gpio_value_dev_t);
	error5:
		class_destroy(class_a13_gpio_config);
	error4:
		cdev_del(&a13_dev.a13_cdev_value);
	error3:
		unregister_chrdev_region(a13_gpio_value_dev_t, 1);
	error2:
		cdev_del(&a13_dev.a13_cdev_config);
	error1:
		unregister_chrdev_region(a13_gpio_config_dev_t, 1);
}
static int __init a13_gpio_init(void) {

	pr_err("a13_gpio_init start\n");

	register_and_create_devices();

	//mmap MMIO region
	if( request_mem_region(A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE, "a13_gpio_memory") == NULL )
	{
		pr_err("a13_gpio error: unable to obtain I/O memory address:%#x\n",A13_GPIO_BASE_ADDRESS);
	}
	else {
		pr_err("a13_gpio I/O memory address:%#x obtain success\n", A13_GPIO_BASE_ADDRESS);
	}
	sunxi_gpio = (struct sunxi_gpio_reg*)ioremap( A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE);
	if((unsigned long*)sunxi_gpio == NULL) {
		pr_err("Unable to remap gpio memory");
		goto error;
	}
	else {
		pr_err("a13_gpio I/O memory remap address:0x%p\n", sunxi_gpio);
	}

	return 0;

	error:
		release_mem_region(A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE);

	return -1;
}

static void __exit a13_gpio_exit(void) {
	pr_err("a13_gpio_exit\n");

	deregister_and_delete_devices ();

	release_mem_region(A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE);
	iounmap((unsigned long *)sunxi_gpio);

	return;
}

module_init(a13_gpio_init);
module_exit(a13_gpio_exit);

MODULE_DESCRIPTION("a simple a13_gpio driver");
MODULE_AUTHOR("ilia paev");
MODULE_LICENSE("GPL");

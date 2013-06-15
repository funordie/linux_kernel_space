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

// DEFINITIONS

//#define PRINT_DEBUG_MESSAGES

#define MAXSIZE 41

#define A13_GPIO_BASE_ADDRESS 0x01C20800
#define A13_GPIO_ADDRESS_SIZE 1024

#define __FILE_NAME__ strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__

#define PRINT(_fmt, ...)  \
	printk("[file:%s, line:%d]: " _fmt"\n", __FILE_NAME__, __LINE__ , ##__VA_ARGS__)

#ifdef PRINT_DEBUG_MESSAGES
#define PRINT_DEBUG(_fmt, ...)  \
	printk("[file:%s, line:%d]: " _fmt"\n", __FILE_NAME__, __LINE__ , ##__VA_ARGS__)
#else
#define PRINT_DEBUG(_fmt, ...)
#endif

// TYPES
struct a13_dev_struct {
	struct cdev a13_cdev_lcd;
};

struct a13_dev_struct a13_dev;

//lcd device
static dev_t a13_lcd_dev_t = MKDEV(255, 253);

static struct class *class_a13_lcd;

static struct device *dev_a13_lcd;

struct sunxi_gpio_reg *sunxi_gpio;

static unsigned long ulReadCount = 0;

/*////////////////////////////////////////////  file_operations - value ////////////////////////////////////////////*/
static	ssize_t a13_lcd_open (struct inode *inode, struct file *file)
{
	PRINT_DEBUG("a13_lcd_open");
	ulReadCount = 0;
	return 0;
}
static loff_t a13_lcd_llseek(struct file *file, loff_t offset, int whence)
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

static ssize_t a13_lcd_read(struct file *file, char __user *buf, size_t size, loff_t *offset) {
//	unsigned long  port;
//	struct sunxi_gpio * gpio_bank_pointer;
//	char ch[10] = "test\n";

	PRINT_DEBUG("a13_lcd_read size:%d offset:%lld file->f_pos:%lld", size , (long long)*offset, (long long)file->f_pos);

/*
	if(ulReadCount != 0) return 0;

	ulReadCount++;

	//set port
	port = *offset >> 2;

	//get selected bank pointer
	gpio_bank_pointer = &sunxi_gpio->gpio_bank[port];

	copy_to_user(buf, &gpio_bank_pointer->dat, sizeof(gpio_bank_pointer->dat));

	PRINT_DEBUG("a13_gpio_value_read result address:0x%p value:%#x size:%d", &gpio_bank_pointer->dat , gpio_bank_pointer->dat, sizeof(gpio_bank_pointer->dat));

	return sizeof(gpio_bank_pointer->dat);
	*/
	return 0;
}

void SetGpioPinDirection(char cPort, unsigned short usPin, unsigned short usValue)
{
	unsigned long port;
	struct sunxi_gpio * gpio_bank_pointer;

	//set pin configurations
	//input data format: A BB C
	// A - port number [A - I]
	// BB - pin number [00 - 31]
	// C - direction [0-input 1-output]
	//example: G 09 0 - G9 direction output

	switch(cPort) {
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
		return;
	}

	gpio_bank_pointer = &sunxi_gpio->gpio_bank[port];

	if(usValue) {
		//set direction output
		gpio_bank_pointer->cfg[usPin >> 3] |= (1 << ((usPin-((usPin>>3)*8))*4));
	}
	else {
		//set direction input
		gpio_bank_pointer->cfg[usPin >> 3] &= ~(1 << ((usPin-((usPin>>3)*8))*4));
	}
	PRINT_DEBUG("a13_gpio_config_write after write address:%p value:%#018x\n", &gpio_bank_pointer->cfg[usPin >> 3], gpio_bank_pointer->cfg[usPin >> 3]);
}
static void SetGpioPinValue(char cPort, unsigned short usPin, unsigned short usValue) {

	unsigned long port;
	struct sunxi_gpio * gpio_bank_pointer;

	PRINT_DEBUG("SetGpioPinValue PORT:%c PIN:%d VALUE:%d", cPort, usPin, usValue);

	//NOTE ALL OUTPUTS PINS ARE INVERTED AFTER 3.3V to 5V CONVERTION
	usValue = !usValue;

	//set pin value
	//input data format: A BB C
	// A - port number [A - I]
	// BB - pin number [00 - 31]
	// C - value [0-off 1-on]
	//example: G 09 1 - G9 pin command:ON

	switch(cPort) {
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
			return;
	}

	gpio_bank_pointer = &sunxi_gpio->gpio_bank[port];

	if(usValue != 0) {
		//set ON
		gpio_bank_pointer->dat |= (1 << usPin);
	}
	else {
		//set OFF
		gpio_bank_pointer->dat &= ~(1 << usPin);
	}
	PRINT_DEBUG("SetGpioPinValue after write port:%lu address:%p value:%#018x",port, &gpio_bank_pointer->dat, gpio_bank_pointer->dat);
}

//	pin 03 - DB0 E 11
//	pin 04 - BD1 B 04
//	pin 05 - DB2 E 10
//	pin 06 - DB3 B 10
//	pin 07 - DB4 E 09
//	pin 08 - DB5 E 08
//	pin 09 - DB6 E 07
//	pin 10 - DB7 E 06

void LCD_WriteData(unsigned short usData) {

	PRINT_DEBUG("LCD_WriteData usValue:%#X", usData);

    //data pin00
	SetGpioPinValue('E', 11, usData & 0x01);

	//data pin01
	SetGpioPinValue('B', 4, usData & 0x02);

	//data pin02
	SetGpioPinValue('E', 10, usData & 0x04);

	//data pin03
	SetGpioPinValue('B', 10, usData & 0x08);

	//data pin04
	SetGpioPinValue('E', 9, usData & 0x10);

	//data pin05
	SetGpioPinValue('E', 8, usData & 0x20);

	//data pin06
	SetGpioPinValue('E', 7, usData & 0x40);

	//data pin06
	SetGpioPinValue('E', 6, usData & 0x80);
}

void delay(unsigned short usMilisecDelay) {
	volatile int i;

	for(i = 0; i < usMilisecDelay*100000; i++) {}
}

//	pin 01 - RS G 09
//	pin 02 - E  B 03
void LCD_SendCommand(unsigned short usValue) {

	PRINT_DEBUG("LCD_SendCommand usValue:%d", usValue);

	if(usValue) {
		//data
		SetGpioPinValue('G', 9, 1);
	}
	else {
		//instruction
		SetGpioPinValue('G', 9, 0);
	}
	//set E = 0
	SetGpioPinValue('B', 3, 0);

	delay(1);

	//set E = 1
	SetGpioPinValue('B', 3, 1);

	//delay
	delay(1);

	//set E = 0
	SetGpioPinValue('B', 3, 0);

	//delay
	delay(1);
}
void printlcd(char * p) {

	int nCount = 0;
	LCD_WriteData(0x1);
	LCD_SendCommand(0);

	LCD_WriteData(0x2);
	LCD_SendCommand(0);


	while(*p) {

		if(nCount == 20) {
			//set DDRAM address to line1(address ox40)
			LCD_WriteData(0xC0);
			LCD_SendCommand(0);

		}

		PRINT_DEBUG("a13_lcd printlcd:%#X nCount:%d", *p, nCount);
		//sed data pins
		LCD_WriteData(*p);

		//send data command
		LCD_SendCommand(1);
		p++;
		nCount++;
	}
}
static	ssize_t a13_lcd_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{

	char d_buf[MAXSIZE];

	PRINT_DEBUG("a13_lcd_write buf:[%#X %#X %#X %#X] size:%d offset:%lld"
			, buf[0] , buf[1], buf[2], buf[3] , size , *offset);

    if(size < MAXSIZE) {
        copy_from_user(d_buf, buf, size);
        d_buf[size-1] = 0;
        printlcd(d_buf);
        *offset += size;
        return size;
        }
    else {
        copy_from_user(d_buf, buf, MAXSIZE - 1);
        d_buf[MAXSIZE - 1] = 0;
        printlcd(d_buf);
        *offset += MAXSIZE - 1;
        //return MAXSIZE - 1;
        return -ENOSPC;
    }
}

static	int a13_lcd_release (struct inode *inode, struct file *file)
{
	PRINT_DEBUG("a13_lcd_release");
	return 0;
}

static const struct file_operations a13_lcd_fops = {
	.owner = THIS_MODULE,
	.open		= a13_lcd_open,
	.read 		= a13_lcd_read,
	.llseek		= a13_lcd_llseek,
	.write		= a13_lcd_write,
	.release	= a13_lcd_release
};

static void deregister_and_delete_devices (void) {

	//delete devices
	device_destroy(class_a13_lcd, a13_lcd_dev_t);
	class_destroy(class_a13_lcd);

	//unregister devices
	cdev_del(&a13_dev.a13_cdev_lcd);
	unregister_chrdev_region(a13_lcd_dev_t, 1);
}

static void register_and_create_devices (void) {

	void *ptr_err;

	//register device
	if (register_chrdev_region(a13_lcd_dev_t, 1, "a13_lcd_device")) {
		PRINT("a13_lcd:Failed to allocate lcd device number");
		return;
	}
	cdev_init(&a13_dev.a13_cdev_lcd, &a13_lcd_fops);
	if (cdev_add(&a13_dev.a13_cdev_lcd, a13_lcd_dev_t, 1)) {
		PRINT("a13_lcd: Failed to add lcd device number");
		goto error1;
	}

	//create lcd device
	class_a13_lcd = class_create(THIS_MODULE, "a13_lcd_class");
	if (IS_ERR(ptr_err = class_a13_lcd))
		goto error2;
	dev_a13_lcd = device_create(class_a13_lcd, NULL, a13_lcd_dev_t, NULL, "a13_lcd");
	if (IS_ERR(ptr_err = dev_a13_lcd))
		goto error3;

	return;


	error3:
		class_destroy(class_a13_lcd);
	error2:
		cdev_del(&a13_dev.a13_cdev_lcd);
	error1:
		unregister_chrdev_region(a13_lcd_dev_t, 1);
}

static void init_lcd_display(void) {
	//	pin 01 - RS G 09
	//	pin 02 - E  B 03
	//	pin 03 - DB0 E 11
	//	pin 04 - BD1 B 04
	//	pin 05 - DB2 E 10
	//	pin 06 - DB3 B 10
	//	pin 07 - DB4 E 09
	//	pin 08 - DB5 E 08
	//	pin 09 - DB6 E 07
	//	pin 10 - DB7 E 06

	SetGpioPinDirection('G' , 9, 1);
	SetGpioPinDirection('B' , 3, 1);

	SetGpioPinDirection('E' ,11, 1);
	SetGpioPinDirection('B' , 4, 1);
	SetGpioPinDirection('E' ,10, 1);
	SetGpioPinDirection('B' ,10, 1);
	SetGpioPinDirection('E' , 9, 1);
	SetGpioPinDirection('E' , 8, 1);
	SetGpioPinDirection('E' , 7, 1);
	SetGpioPinDirection('E' , 6, 1);

	//send instructions
	LCD_WriteData(0x38);
	LCD_SendCommand(0);

	LCD_WriteData(0x38);
	LCD_SendCommand(0);

	LCD_WriteData(0x38);
	LCD_SendCommand(0);

	LCD_WriteData(0x0C);
	LCD_SendCommand(0);

	LCD_WriteData(0x06);
	LCD_SendCommand(0);

	LCD_WriteData(0x1);
	LCD_SendCommand(0);

	LCD_WriteData(0x2);
	LCD_SendCommand(0);
}
static int __init a13_lcd_driver_init(void) {

	PRINT("a13_lcd_driver_init start");

	register_and_create_devices();

	//mmap MMIO region
	if( request_mem_region(A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE, "a13_lcd_gpio_memory") == NULL )
	{
		PRINT("a13_lcd error: unable to obtain I/O memory address:%#x",A13_GPIO_BASE_ADDRESS);
	}
	else {
		PRINT("a13_lcd I/O memory address:%#x obtain success", A13_GPIO_BASE_ADDRESS);
	}
	sunxi_gpio = (struct sunxi_gpio_reg*)ioremap( A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE);
	if((unsigned long*)sunxi_gpio == NULL) {
		PRINT("Unable to remap gpio memory");
		goto error;
	}
	else {
		PRINT("a13_lcd I/O memory remap address:0x%p", sunxi_gpio);
	}

	//init display
	init_lcd_display();

	return 0;

	error:
		release_mem_region(A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE);

	return -1;
}

static void __exit a13_lcd_driver_exit(void) {
	PRINT("a13_lcd_driver_exit");

	deregister_and_delete_devices ();

	release_mem_region(A13_GPIO_BASE_ADDRESS, A13_GPIO_ADDRESS_SIZE);
	iounmap((unsigned long *)sunxi_gpio);

	return;
}

module_init(a13_lcd_driver_init);
module_exit(a13_lcd_driver_exit);

MODULE_DESCRIPTION("a simple a13 HD44780 driver");
MODULE_AUTHOR("ilia paev");
MODULE_LICENSE("GPL");

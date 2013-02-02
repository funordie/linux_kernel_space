#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/moduleparam.h>

static char *whom = "world";
static int howmany = 1;

module_param(howmany, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);

static int __init mychardev_start(void)
{
	int i;
	printk(KERN_INFO "Loading mychardev module...\n");
	for(i=0;i<howmany;i++)
		printk(KERN_INFO "Hello %s\n", whom);
	return 0;
}
static void __exit mychardev_end(void)
{
	int i;
	for(i=0;i<howmany;i++)
		printk(KERN_INFO "Goodbye Mr.%s\n",whom);
}
module_init(mychardev_start);
module_exit(mychardev_end);

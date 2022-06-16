#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

static int itype=0;
module_param(itype, int, 0);

int my_add(int a, int b)
{
	return a+b;
}

EXPORT_SYMBOL(my_add);

/*
static int __init hello_init(void)
{
	printk("[ default ] itype = %d", itype);
	printk(KERN_EMERG "[ KERN_EMERG ] Hello module init\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk("[ default ] Hello module exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
*/

MODULE_LICENSE("GPL2");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("hello module");
MODULE_ALIAS("test hello");


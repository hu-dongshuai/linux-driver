#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include "caculation.h"

static int itype=0;
module_param(itype, int, 0);

static int __init hello_init(void)
{
	printk("[ default ] itype = %d, myadd %d", itype,my_add(1, itype));
	printk(KERN_EMERG "[ KERN_EMERG ] Hello module init\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk("[ default ] Hello module exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL2");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("hello module");
MODULE_ALIAS("test hello");


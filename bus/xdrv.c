#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
extern struct bus_type xbus;
char *name = "xdrv";

ssize_t drvname_show(struct device_driver *drv, char *buf)
{
	return sprintf(buf, "%s\n", name);
}

DRIVER_ATTR_RO(drvname);

int xdrv_probe(struct device *dev)
{
	printk("%s-%s\n", __FILE__, __func__);
	return 0;
}

int xdrv_remove(struct device *dev)
{
	printk("%s-%s\n", __FILE__, __func__);
	return 0;
}

static struct device_driver xdrv = {
	.name = "xdev",
	.bus = &xbus,
	.probe = xdrv_probe,
	.remove = xdrv_remove,
};

static int __init xdrv_init(void)
{
	printk("[ default ] xdrv init");
	driver_register(&xdrv);
	driver_create_file(&xdrv, &driver_attr_drvname);
	return 0;
}

static void __exit xdrv_exit(void)
{
	printk("[ default ] xdrv exit\n");
	driver_remove_file(&xdrv, &driver_attr_drvname);
	driver_unregister(&xdrv);
}

module_init(xdrv_init);
module_exit(xdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("xbus module");
MODULE_ALIAS("test xbus");


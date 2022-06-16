#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

static char *bus_name = "xbus";

ssize_t xbus_test_show(struct bus_type *bus, char *buf)
{
	return sprintf(buf, "%s\n", bus_name);
}

BUS_ATTR(xbus_test, S_IRUSR, xbus_test_show, NULL);

int xbus_match(struct device *dev, struct device_driver *drv)
{
	printk("[ bus ] %s-%s\n", __FILE__, __func__);

	if (!strncmp(dev_name(dev), drv->name, strlen(drv->name))){
		printk("[ bus ] dev and drv match\n");
		return 1;
	}
	return 0;
}

static struct bus_type xbus =
{
	.name = "xbus", 
	.match = xbus_match,
};

EXPORT_SYMBOL(xbus);

static int __init xbus_init(void)
{
	printk("[ default ] xbus init");
	bus_register(&xbus);
	bus_create_file(&xbus, &bus_attr_xbus_test);
	return 0;
}

static void __exit xbus_exit(void)
{
	printk("[ default ] xbus exit\n");
	bus_remove_file(&xbus, &bus_attr_xbus_test);
	bus_unregister(&xbus);
}

module_init(xbus_init);
module_exit(xbus_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("xbus module");
MODULE_ALIAS("test xbus");


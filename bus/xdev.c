#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

extern struct bus_type xbus;
unsigned long id = 0;

ssize_t xdev_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%ld\n", id);
}

ssize_t xdev_id_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
	kstrtoul(buf, 10, &id);
	return count;
}
DEVICE_ATTR(xdev_id, S_IRUSR|S_IWUSR, xdev_id_show, xdev_id_store);


void xdev_release(struct device *dev)
{
	printk("[ xdev ] %s-%s\n", __FILE__, __func__);
}


static struct device xdev = {
	.init_name = "xdev",
	.bus = &xbus,
	.release = xdev_release,
};



static int __init xbus_init(void)
{
	printk("[ default ] xdev init");
	device_register(&xdev);
	device_create_file(&xdev, &dev_attr_xdev_id);
	return 0;
}

static void __exit xbus_exit(void)
{
	printk("[ default ] xdev exit\n");
	device_remove_file(&xdev, &dev_attr_xdev_id);
	device_unregister(&xdev);
}

module_init(xbus_init);
module_exit(xbus_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("xbus module");
MODULE_ALIAS("test xbus");


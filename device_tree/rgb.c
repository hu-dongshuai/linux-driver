#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_address.h>
static struct of_device_id rgb_led[] = {
	{.name = "rgb_led"},
	{}
};

struct led_data
{
	struct device_node *device_node;
	unsigned int __iomem *va_MODER;
	unsigned int __iomem *va_OTYPER;
	unsigned int __iomem *va_OSPEEDR;
	unsigned int __iomem *va_PUPDR;
	unsigned int __iomem *va_BSRR;

	unsigned int led_pin;
	struct cdev led_cdev;

};
struct device_node *rgb_led_device_node;
static dev_t devno;
struct class *myled_class;
static void  __iomem *va_clkaddr;

static int led_open(struct inode *inode, struct file *filp)
{
	unsigned val = 0;
	struct led_data *cdev = (struct led_data*)container_of(inode->i_cdev, struct led_data, led_cdev);
	val |= (0x43);
	iowrite32(val, va_clkaddr);


	val = ioread32(cdev->va_MODER);
	val &= ~((unsigned int)0x3 << (2*cdev->led_pin));
	val |= ((unsigned int)0x01 << (2*cdev->led_pin));
	iowrite32(val, cdev->va_MODER);

	val = ioread32(cdev->va_OTYPER);
	val &= ~((unsigned int)0x1 << (cdev->led_pin));
	iowrite32(val, cdev->va_OTYPER);

	val = ioread32(cdev->va_OSPEEDR);
	val &= ~((unsigned int)0x3 << (2*cdev->led_pin));
	val |= ((unsigned int)0x02 << (2*cdev->led_pin));
	iowrite32(val, cdev->va_OSPEEDR);

	val = ioread32(cdev->va_PUPDR);
	val &= ~((unsigned int)0x3 << (2*cdev->led_pin));
	val |= ((unsigned int)0x01 << (2*cdev->led_pin));
	iowrite32(val, cdev->va_PUPDR);

	val = ioread32(cdev->va_BSRR);
	val |= ((unsigned int)0x01 << (cdev->led_pin + 16));
	iowrite32(val, cdev->va_BSRR);

	filp->private_data = cdev;
	return 0;
}

static ssize_t led_chrdev_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long val = 0;
	unsigned long ret = 0;

	int tmp = count;
	struct led_data *cdev = (struct led_data *)filp->private_data;
	kstrtoul_from_user(buf, tmp, 10, &ret);

	if (ret == 0)
	{
		val |= (0x01 << (cdev->led_pin+16));
	} else
	{
		val |= (0x01 << cdev->led_pin);
	}

	iowrite32(val, cdev->va_BSRR);
	*ppos += tmp;
	return tmp;
}
static int led_close(struct inode *inode, struct file *filp)
{
	printk("[ led ] close pin \n");

	return 0;
}

static struct file_operations led_chrdev_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_close,
	.write = led_chrdev_write,
};

static int led_pdrv_probe(struct platform_device *pdev)
{

	struct led_data *cur_led;
	dev_t cur_dev;

	cur_led = devm_kzalloc(&pdev->dev, sizeof(struct led_data), GFP_KERNEL);
	if (!cur_led)
		return -ENOMEM;

	printk("led platform driver probe\n");
	rgb_led_device_node = of_find_node_by_path("/rgb_led");
	if (rgb_led_device_node == NULL)
	{
		printk("[ rgb ] get rgb node failed \n");
		return -1;
	}

	cur_led->device_node = of_find_node_by_name(rgb_led_device_node, "rgb_led_red");
	 if (cur_led->device_node == NULL)
	 {
	 	printk("[ rgb ] get red node failed\n");
	 	return -1;
	 }

	 cur_led->va_MODER = of_iomap(cur_led->device_node, 0);
	 cur_led->va_OTYPER = of_iomap(cur_led->device_node, 1);
	 cur_led->va_OSPEEDR = of_iomap(cur_led->device_node, 2);
	 cur_led->va_PUPDR = of_iomap(cur_led->device_node, 3);
	 cur_led->va_BSRR = of_iomap(cur_led->device_node, 4);
	 cur_led->led_pin = 13;
	 va_clkaddr = of_iomap(cur_led->device_node, 5);


	alloc_chrdev_region(&devno, 0, 1, "led_dev");
	cdev_init(&cur_led->led_cdev, &led_chrdev_fops);
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	cdev_add(&cur_led->led_cdev, cur_dev, 1);
	device_create(myled_class, NULL, cur_dev, NULL, "led_dev");
	
	platform_set_drvdata(pdev, cur_led);
	return 0;
}

static int led_pdrv_remove( struct platform_device *pdev)
{
	dev_t cur_dev;
	
	struct led_data *cur_data = platform_get_drvdata(pdev);
	
	printk("led platform driver remove\n");

	cur_dev = MKDEV(MAJOR(devno), pdev->id);
	cdev_del(&cur_data->led_cdev);
	device_destroy(myled_class, cur_dev);
	unregister_chrdev_region(cur_dev, 1);
	return 0;
}

static struct platform_driver led_pdrv = {
	.probe = led_pdrv_probe,
	.remove = led_pdrv_remove,
	.driver = {
		.name = "rgb_led_platform",
		.owner = THIS_MODULE,
		.of_match_table = rgb_led,
	}
};

static __init int led_pdrv_init(void)
{
	platform_driver_register(&led_pdrv);
	return 0;
}

static __exit void led_pdrv_exit(void)
{
	platform_driver_unregister(&led_pdrv);
	class_destroy(myled_class);
}
module_init(led_pdrv_init);
module_exit(led_pdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("hello module");
MODULE_ALIAS("test hello");


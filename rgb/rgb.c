#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

struct led_cdev
{
    struct cdev dev;
	unsigned int __iomem *va_moder;
	unsigned int __iomem *va_otyper;
	unsigned int __iomem *va_ospeedr;
	unsigned int __iomem *va_pupdr;
	unsigned int __iomem *va_bsrr;
	unsigned int  led_pin;
};

static struct led_cdev led_cdev = {
	.led_pin = 	13,
};

static dev_t devno;
struct class *led_chrdev_class;
unsigned int __iomem *va_clkaddr;

#define AHB4 0x50000000
#define RCC_OCENSETR (AHB4 + 0xA28)
#define GPIOA 0x50002000

#define GPIOA_PORT_MOD GPIOA
#define GPIOA_OUTPUT_TYPE (GPIOA + 0x04)
#define GPIOA_OUTPUT_SPEED (GPIOA + 0x08)
#define GPIOA_PULL_UP_DOWN (GPIOA + 0x0c)
#define GPOIA_INPUT_DATA (GPIOA + 0x10)
#define GPIOA_OUTPUT_DATA (GPIOA + 0x14)
#define GPIOA_PORT_SET_RESET (GPIOA + 0x18)


static int led_open(struct inode *inode, struct file *filp)
{
	unsigned val = 0;
	struct led_cdev *cdev = (struct led_cdev*)container_of(inode->i_cdev, struct led_cdev, dev);
	filp->private_data = cdev;
	printk("[ led ] open pin %d\n", cdev->led_pin);
	va_clkaddr = ioremap(RCC_OCENSETR, 4);
	val |= (0x43);
	iowrite32(val, va_clkaddr);


	val = ioread32(cdev->va_moder);
	val &= ~((unsigned int)0x3 << (2*cdev->led_pin));
	val |= ((unsigned int)0x01 << (2*cdev->led_pin));
	iowrite32(val, cdev->va_moder);

	val = ioread32(cdev->va_otyper);
	val &= ~((unsigned int)0x1 << (cdev->led_pin));
	iowrite32(val, cdev->va_otyper);

	val = ioread32(cdev->va_ospeedr);
	val &= ~((unsigned int)0x3 << (2*cdev->led_pin));
	val |= ((unsigned int)0x02 << (2*cdev->led_pin));
	iowrite32(val, cdev->va_ospeedr);

	val = ioread32(cdev->va_pupdr);
	val &= ~((unsigned int)0x3 << (2*cdev->led_pin));
	val |= ((unsigned int)0x01 << (2*cdev->led_pin));
	iowrite32(val, cdev->va_pupdr);

	val = ioread32(cdev->va_bsrr);
	val |= ((unsigned int)0x01 << (cdev->led_pin + 16));
	iowrite32(val, cdev->va_bsrr);
	return 0;
}

static ssize_t led_chrdev_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long val = 0;
	unsigned long ret = 0;

	int tmp = count;
	struct led_cdev *cdev = (struct led_cdev *)filp->private_data;
	kstrtoul_from_user(buf, tmp, 10, &ret);

	if (ret == 0)
	{
		val |= (0x01 << (cdev->led_pin+16));
	} else
	{
		val |= (0x01 << cdev->led_pin);
	}

	iowrite32(val, cdev->va_bsrr);
	*ppos += tmp;
	return tmp;
}
static int led_close(struct inode *inode, struct file *filp)
{
	printk("[ led ] close pin \n");
	iounmap(va_clkaddr);
	return 0;
}

static struct file_operations led_chrdev_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_close,
	.write = led_chrdev_write,
};

static int __init hello_init(void)
{
	dev_t cur_dev;

	printk("[ default ] cdev init");
	alloc_chrdev_region(&devno, 0, 1, "led_dev");

	led_cdev.va_moder = ioremap(GPIOA_PORT_MOD, 4);
	led_cdev.va_bsrr = ioremap(GPIOA_PORT_SET_RESET, 4);
	led_cdev.va_pupdr = ioremap(GPIOA_PULL_UP_DOWN, 4);
	led_cdev.va_ospeedr = ioremap(GPIOA_OUTPUT_SPEED, 4);
	led_cdev.va_otyper = ioremap(GPIOA_OUTPUT_TYPE, 4);
	
	cdev_init(&led_cdev.dev, &led_chrdev_fops);
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	cdev_add(&led_cdev.dev, cur_dev, 1);

	led_chrdev_class = class_create(THIS_MODULE, "led_chrdev");
	device_create(led_chrdev_class, NULL, cur_dev, NULL, "led_dev");
	printk(KERN_EMERG "[ KERN_EMERG ] Hello module init\n");
	return 0;
}

static void __exit hello_exit(void)
{
	dev_t cur_dev;
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	printk("[ default ] Led module exit\n");

	iounmap(led_cdev.va_moder);
	iounmap(led_cdev.va_bsrr);
	iounmap(led_cdev.va_pupdr);
	iounmap(led_cdev.va_ospeedr);
	iounmap(led_cdev.va_otyper);

	device_destroy(led_chrdev_class, cur_dev);
	cdev_del(&led_cdev.dev);
	unregister_chrdev_region(devno, 1);
	class_destroy(led_chrdev_class);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("hello module");
MODULE_ALIAS("test hello");


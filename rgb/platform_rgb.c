#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

static struct platform_device_id led_pdev_ids[] = {
	{.name = "led_pdev"},
	{}
};

struct led_data
{
	unsigned int led_pin;
	unsigned int clk_regshift;
	unsigned int __iomem *va_MODER;
	unsigned int __iomem *va_OTYPER;
	unsigned int __iomem *va_OSPEEDR;
	unsigned int __iomem *va_PUPDR;
	unsigned int __iomem *va_BSRR;

	struct cdev led_cdev;

};

static dev_t devno;
struct class *myled_class;
unsigned int __iomem *va_clkaddr;

static int led_open(struct inode *inode, struct file *filp)
{
	unsigned val = 0;
	struct led_data *cdev = (struct led_data*)container_of(inode->i_cdev, struct led_data, led_cdev);
	printk("[ led ] open pin %d\n", cdev->led_pin);
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
	unsigned int *led_hwinfo;

	struct resource *mem_MODER;
	struct resource *mem_OTYPER;
	struct resource *mem_OSPEEDR;
	struct resource *mem_PUPDR;
	struct resource *mem_BSRR;
	struct resource *mem_CLK;

	dev_t cur_dev;
	printk("led platform driver probe\n");
	cur_led = devm_kzalloc(&pdev->dev, sizeof(struct led_data), GFP_KERNEL);
	if (!cur_led)
		return -ENOMEM;

	led_hwinfo = devm_kzalloc(&pdev->dev, sizeof(unsigned int)*2, GFP_KERNEL);
	if (!led_hwinfo)
		return -ENOMEM; //need to free cur_led

	led_hwinfo = dev_get_platdata(&pdev->dev);

	cur_led->led_pin = led_hwinfo[0];
	cur_led->clk_regshift = led_hwinfo[1];

	mem_MODER = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mem_OTYPER = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	mem_OSPEEDR = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	mem_PUPDR = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	mem_BSRR = platform_get_resource(pdev, IORESOURCE_MEM, 4);
	mem_CLK = platform_get_resource(pdev, IORESOURCE_MEM, 5);

	cur_led->va_MODER = devm_ioremap(&pdev->dev, mem_MODER->start, resource_size(mem_MODER));
	cur_led->va_OTYPER = devm_ioremap(&pdev->dev, mem_OTYPER->start, resource_size(mem_OTYPER));
	cur_led->va_OSPEEDR = devm_ioremap(&pdev->dev, mem_OSPEEDR->start, resource_size(mem_OSPEEDR));
	cur_led->va_PUPDR = devm_ioremap(&pdev->dev, mem_PUPDR->start, resource_size(mem_PUPDR));
	cur_led->va_BSRR = devm_ioremap(&pdev->dev, mem_BSRR->start, resource_size(mem_BSRR));
	va_clkaddr = devm_ioremap(&pdev->dev, mem_CLK->start, resource_size(mem_CLK));

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
	iounmap(va_clkaddr);	
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
	.driver.name = "led_pdev",
	.id_table = led_pdev_ids,
};

static __init int led_pdrv_init(void)
{
	myled_class = class_create(THIS_MODULE, "my_leds");
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


#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>

static struct of_device_id of_button[] = {
	{.compatible = "button_interrupt"},
	{}
};

struct button_drv
{
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *node;
	unsigned int irq;
	unsigned int gpio;
};
dev_t devno;
static irqreturn_t irq_handler(int irq, void *dev_id)
{
	printk("[ button ] irq num %d", irq);
	return 0;
}
static int open(struct inode *inode, struct file *filp)
{
	int ret;
	struct button_drv *cdev = (struct button_drv*)container_of(inode->i_cdev, struct button_drv, cdev);
	filp->private_data = cdev;
	cdev->node = of_find_node_by_path("/button_interrupt");
	if (NULL == cdev->node)
	{
		printk("[ button ] failed to find node\n");
		return -1;
	}

	cdev->gpio = of_get_named_gpio(cdev->node, "button_gpio", 0);
	if (0 == cdev->gpio)
	{
		printk("[ button ] Failed to get gpio\n");
		return -1;
	}

	// ret = gpio_request(cdev->gpio, "button_gpio");
	// if (ret < 0)
	// {
	// 	printk("[ button ] gpio request error\n");
	// 	gpio_free(cdev->gpio);
	// 	return -1;
	// }

	ret = gpio_direction_input(cdev->gpio);
	cdev->irq = irq_of_parse_and_map(cdev->node, 0);

	ret = request_irq(cdev->irq, irq_handler, IRQF_TRIGGER_RISING, "button_iunterrupt",cdev );
	if (ret != 0)
	{
		printk("[ button ] rqeuest irq error\n");
		free_irq(cdev->irq, cdev);
	}


	return 0;
}

static int button_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{

	return 0;
}
static int close(struct inode *inode, struct file *filp)
{
	struct button_drv *cdev = (struct button_drv*)container_of(inode->i_cdev, struct button_drv, cdev);
	//gpio_free(cdev->gpio);
	free_irq(cdev->irq, cdev);
	return 0;
}

static struct file_operations button_chrdev_fops = {
	.owner = THIS_MODULE,
	.open = open,
	.read = button_read,
	.release = close,
};

static int pdrv_probe(struct platform_device *pdev)
{

	int ret; 
	struct button_drv *button;
	dev_t cur_dev;

	printk("[ button ] platform driver probe\n");

	ret = alloc_chrdev_region(&devno, 0, 1, "oled_dev");
	if (ret < 0)
	{
		printk("[ button ] Failed to alloc chardev  \n");
		return -ENOMEM;
	}
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	button = devm_kzalloc(&pdev->dev, sizeof(struct button_drv), GFP_KERNEL);
	if (!button)
		return -ENOMEM;
	
	cdev_init(&button->cdev, &button_chrdev_fops);
	
	ret = cdev_add(&button->cdev, cur_dev, 1);
	if (ret < 0)
	{
		printk("[ button ] Failed to add cdev\n");
		goto del_unregister;
	}

	button->class =  class_create(THIS_MODULE, "button_class");
	if ( !button->class)
	{
		printk("[ button ] Failed to create class\n");
		goto del_cdev;
	}

	button->device = device_create(button->class, NULL, cur_dev, NULL, "button_dev");
	if ( !button->device)
	{
		printk("[ button ] Failed to create class\n");
		goto del_class;
	}

	platform_set_drvdata(pdev, button);
	return 0;

del_class:
	class_destroy(button->class);
del_cdev:
	cdev_del(&button->cdev);
del_unregister:
	unregister_chrdev_region(cur_dev, 1);

	return ret;
}

static int pdrv_remove( struct platform_device *pdev)
{
	dev_t cur_dev;
	
	struct button_drv *button = platform_get_drvdata(pdev);
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	device_destroy(button->class, cur_dev);
	class_destroy(button->class);
	cdev_del(&button->cdev);
	unregister_chrdev_region(cur_dev, 1);
	return 0;
}

static struct platform_driver oled_driver = {
	.probe = pdrv_probe,
	.remove = pdrv_remove,
	.driver = {
		.name = "button",
		.owner = THIS_MODULE,
		.of_match_table = of_button,
	}
};

static __init int pdrv_init(void)
{
	return platform_driver_register(&oled_driver);

}

static __exit void pdrv_exit(void)
{
	platform_driver_unregister(&oled_driver);
}
module_init(pdrv_init);
module_exit(pdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("oled module");
MODULE_ALIAS("oled driver");


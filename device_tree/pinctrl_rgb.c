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
#include <linux/of_gpio.h>
static struct of_device_id rgb_led[] = {
	{.name = "rgb_led"},
	{}
};

struct led_data
{
	unsigned int led_pin;
	struct cdev led_cdev;
	struct class *myled_class;
	struct device_node *rgb_led_device_node;

};
static dev_t devno;

static int led_open(struct inode *inode, struct file *filp)
{
	struct led_data *cdev = (struct led_data*)container_of(inode->i_cdev, struct led_data, led_cdev);
	filp->private_data = cdev;
	return 0;
}

static ssize_t led_chrdev_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long write_data;
	struct led_data *cur_dev = (struct led_data*)filp->private_data;

	int erro = kstrtoul_from_user(buf, count, 10, &write_data);
	if (erro < 0)
		return -1;
	if (write_data )
	{
		gpio_direction_output(cur_dev->led_pin,0);
		printk("[ rgb ] Light on\n");
	} else 
	{
		gpio_direction_output(cur_dev->led_pin,1);
		printk("[ rgb ] Light off\n");
	}

	*ppos += count;
	return count;
}
static int led_close(struct inode *inode, struct file *filp)
{
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
	cur_led->rgb_led_device_node = of_find_node_by_path("/rgb_led");
	if (cur_led->rgb_led_device_node == NULL)
	{
		printk("[ rgb ] get rgb node failed \n");
		return -1;
	}

	cur_led->led_pin = of_get_named_gpio(cur_led->rgb_led_device_node, "rgb_led_red", 0);
	printk("[ rgb ] led pin %d \n", cur_led->led_pin);

	gpio_direction_output(cur_led->led_pin, 1);
	
	alloc_chrdev_region(&devno, 0, 1, "led_dev");
	cdev_init(&cur_led->led_cdev, &led_chrdev_fops);
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	cdev_add(&cur_led->led_cdev, cur_dev, 1);
	device_create(cur_led->myled_class, NULL, cur_dev, NULL, "led_dev");
	
	platform_set_drvdata(pdev, cur_led);
	return 0;
}

static int led_pdrv_remove( struct platform_device *pdev)
{
	dev_t cur_dev;
	
	struct led_data *cur_led = platform_get_drvdata(pdev);
	
	printk("led platform driver remove\n");

	cur_dev = MKDEV(MAJOR(devno), pdev->id);
	cdev_del(&cur_led->led_cdev);
	device_destroy(cur_led->myled_class, cur_dev);
	class_destroy(cur_led->myled_class);
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
}
module_init(led_pdrv_init);
module_exit(led_pdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("hello module");
MODULE_ALIAS("test hello");


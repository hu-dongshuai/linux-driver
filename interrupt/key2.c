#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/poll.h>

#define KEY_CNT 1
struct key_dev {
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int key_gpio;
	struct timer_list timer;
	int irq_num;

	atomic_t status;
	wait_queue_head_t r_wait;
};
enum key_status {
	KEY_PRESS = 0,
	KEY_RELEASE,
	KEY_KEEP,
};
static struct key_dev key;
static irqreturn_t key_interrupt(int irq, void *dev_id)
{
	mod_timer(&key.timer, jiffies + msecs_to_jiffies(15));
	return 0;
}
static void key_timer_function(struct timer_list *arg)
{
	static int last_val = 1;
	int current_val;

	current_val = gpio_get_value(key.key_gpio);
	
	if (1 == current_val && last_val)
	{
		atomic_set(&key.status, KEY_PRESS);
		wake_up_interruptible(&key.r_wait);
	}
	else if(0 == current_val && !last_val)
	{
		atomic_set(&key.status, KEY_RELEASE);
		wake_up_interruptible(&key.r_wait);
	}
	else 
		atomic_set(&key.status, KEY_KEEP);
	
	last_val = current_val;
}
static int key_parse_dt(void)
{
	int ret = -1;
	const char *str;

	key.nd = of_find_node_by_path("/key_interrupt");
	if (key.nd == NULL)
	{
		printk("[ key ] key node not find\n");
		return ret;
	}

	ret= of_property_read_string(key.nd, "status", &str);
	if (ret < 0)
		return -EINVAL;

	if (strcmp(str, "okay"))
		return -EINVAL;

	ret = of_property_read_string(key.nd, "compatible", &str);
	if (ret < 0)
		return -EINVAL;

	if (strcmp(str, "key_interrupt"))
	{
		printk("[ key ] not match\n");
		return -EINVAL;
	} else {
		printk("[ key ] match!!\n");
	}

	key.key_gpio = of_get_named_gpio(key.nd, "button_gpio", 0);
	if (key.key_gpio < 0)
		return -EINVAL;

	key.irq_num = irq_of_parse_and_map(key.nd, 0);

	if (!key.irq_num)
		return -EINVAL;

	printk("key-gpio num = %d\r\n", key.key_gpio);
	return 0;
}
static int key_gpio_init(void)
{
	int ret;
	unsigned long irq_flags;

	ret = gpio_request(key.key_gpio, "KEY2");
	if (ret)
	{
		printk("[ key ] Failed to request key\n");
		return ret;
	}

	gpio_direction_input(key.key_gpio);

	irq_flags = irq_get_trigger_type(key.irq_num);
	if (IRQF_TRIGGER_NONE == irq_flags)
		irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING;

	ret = request_irq(key.irq_num, key_interrupt, irq_flags, "KEY2_IRQ", &key.devid);
	if (ret)
	{
		gpio_free(key.key_gpio);
		return ret;
	}


	return 0;
}
static int key_open(struct inode *inode, struct file *filp)
{
	printk("[ key ] open key dev\n");
	return 0;
}
static int key_write(struct file* filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}
static int key_release(struct inode *inode, struct file *filp)
{
	return 0;
}
static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret;

	if (filp->f_flags & O_NONBLOCK) {
		if (KEY_KEEP == atomic_read(&key.status))
			return -EAGAIN;
	} else {
		ret = wait_event_interruptible(key.r_wait, KEY_KEEP != atomic_read(&key.status));
		if (ret)
			return ret;
	}

	ret = copy_to_user(buf, &key.status, sizeof(int));
	
	atomic_set(&key.status, KEY_KEEP);

	return ret;
}
static unsigned int key_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(filp, &key.r_wait, wait);

	if (KEY_KEEP != atomic_read(&key.status))
		mask = POLL_IN | POLLRDNORM;
	
	return mask;
}
static struct file_operations key_fops = {
	.owner = THIS_MODULE,
	.open = key_open,
	.read = key_read,
	.write = key_write,
	.release = key_release,
	.poll = key_poll,
};
static __init int pdrv_init(void)
{
	int ret;
	atomic_set(&key.status, KEY_KEEP); 
	init_waitqueue_head(&key.r_wait);

	ret = key_parse_dt();
	if (ret)
		return ret;


	ret = alloc_chrdev_region(&key.devid, 0, KEY_CNT, "key2");
	if (ret < 0)
	{
		printk("Failed to alloc chardev\n");
		goto free_gpio;
	}

	ret = key_gpio_init();
	if (ret)
		return ret;

	key.cdev.owner = THIS_MODULE;
	cdev_init(&key.cdev, &key_fops);
	ret = cdev_add(&key.cdev, key.devid, KEY_CNT);
	if ( ret < 0)
		goto del_unregister;

	key.class = class_create(THIS_MODULE, "key_class");
	if (IS_ERR(key.class)){
		goto del_cdev;
	}

	key.device = device_create(key.class, NULL, key.devid, NULL, "key_dev");
	if (IS_ERR(key.device)){
		goto destory_class;
	}

	timer_setup(&key.timer, key_timer_function, 0);
	return 0;

destory_class:
	class_destroy(key.class);
del_cdev:
	cdev_del(&key.cdev);
del_unregister:
	unregister_chrdev_region(key.devid, KEY_CNT);
free_gpio:
	free_irq(key.irq_num, &key.devid);
	gpio_free(key.key_gpio);
	return ret;
}

static __exit void pdrv_exit(void)
{
	cdev_del(&key.cdev);
	unregister_chrdev_region(key.devid, KEY_CNT);
	del_timer_sync(&key.timer);
	device_destroy(key.class, key.devid);
	class_destroy(key.class);
	free_irq(key.irq_num, &key.devid);
	gpio_free(key.key_gpio);
}
module_init(pdrv_init);
module_exit(pdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("oled module");
MODULE_ALIAS("oled driver");


#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/types.h>

#define X_WIDTH 128
#define Y_WIDTH 64

static struct of_device_id spi_oled[] = {
	{.compatible = "fire,spi_oled"},
	{}
};

struct oled_dev
{
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct spi_device *spi;
	struct device_node *oled_node;
	unsigned int dc_pin;

};
struct oled_display{
	u8 x;
	u8 y;
	u32 length;
	u8 buf[];
};
u8 oled_init_data[] = {
	0xcf, 0x00, 0xc9, 0x30, 0xed,
	0x64, 0x03, 0x12, 0x81, 0xE8,
	0x85, 0x10, 0x7a, 0xcb, 0x39,
	0x2c, 0x00, 0x34, 0x02, 0xf7,
	0x20, 0xea, 0x00, 0x00, 0xc0,
	0x1b, 0xc1, 0x00, 0xc5, 0x30,
	0x30, 0xc7, 0xB7, 0x36, 0x08,
	0x3A, 0x55, 0xB1, 0x00, 0x1a,
	0xb6, 0x0a, 0xa2, 0xf2, 0x00, 
	0x26, 0x01, 0xe0, 0x0f, 0x2a, 
	0x28, 0x08, 0x0e, 0x08, 0x54, 
	0xa9, 0x43, 0x0a, 0x0f, 0x00, 
	0x00, 0x00, 0x00, 0xe1, 0x00, 
	0x15, 0x17, 0x07, 0x11, 0x06, 
	0x2B, 0x56, 0x3C, 0x05, 0x10, 
	0x0f, 0x3f, 0x3f, 0x0f, 0x2b,
	0x00, 0x00, 0x01, 0x3f, 0x2a, 
	0x00, 0x00, 0x00, 0xef, 0x11, 
	0x29

};
static dev_t devno;
static int oled_read(struct  oled_dev *dev, u8 reg, void *buf, int len)
{
	int ret = -1;
	unsigned char txdata[1];
	unsigned char *rxdata;
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = dev->spi;
	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	if (!t)
		return -ENOMEM;
	
	rxdata = kzalloc(sizeof(char)*len, GFP_KERNEL);
	if (!rxdata)
		goto out1;
	txdata[0] = reg | 0x80;
	//t->tx_buf = txdata;
	t->rx_buf = rxdata;
	t->len = len + 1; //发送长度+读取长度
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);
	if (ret)
	{
		printk("[ oled ] Failed to read \n");
		goto out2;
	}

	memcpy(buf, rxdata+1, len);
	kfree(rxdata);
	kfree(t);
	return 0;
out2:
	kfree(rxdata);
out1:
	kfree(t);
	return ret;
}
static int oled_write(struct  oled_dev *dev, u8 reg, int len)
{
	int ret = -1;
	unsigned char *txdata;
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = dev->spi;
	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	if (!t)
		return -ENOMEM;
	
	txdata = kzalloc(sizeof(char)*len, GFP_KERNEL);
	if (!txdata)
		goto out1;
	*txdata = reg;// & ~0x80;
	t->tx_buf = txdata;

	t->len = len;
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);
	if (ret)
	{
		printk("[ oled ] Failed to write \n");
		goto out2;
	}
	kfree(txdata);
	kfree(t);
	return 0;
out2:
	kfree(txdata);
out1:
	kfree(t);
	return ret;
}
static int write(struct file *filp, const char __user *buf, size_t cnt, loff_t *off)
{
	int ret;
	struct oled_display *data;
	//struct oled_dev *cdev = (struct oled_dev *)filp->private_data;
	data = (struct oled_display*)kzalloc(cnt, GFP_KERNEL);
	ret = copy_from_user(data, buf,cnt);

	kfree(data);
	return 0;
}

static int oled_write_reg_data(struct oled_dev *dev, u8 reg, u8 *data, int size)
{
	int i;
	gpio_direction_output(dev->dc_pin, 0);
	oled_write(dev, reg, 1);//write reg
	gpio_direction_output(dev->dc_pin, 1);
	for (i=0; i<size ;i++)
		oled_write(dev, data[i] , 1);//write data

	return 0;
}
static int oled_write_data(struct oled_dev *dev)
{
	int i;
	u8 data[4];
	for (i=0; i < (sizeof(oled_init_data)-1)/4; i++)
	{
		gpio_direction_output(dev->dc_pin, 0);
		oled_write(dev, oled_init_data[i], 1);//write reg
		gpio_direction_output(dev->dc_pin, 1);
		oled_write(dev, oled_init_data[i+1], 1);//write data
		oled_write(dev, oled_init_data[i+2], 1);//write data
		oled_write(dev, oled_init_data[i+3], 1);//write data
	}
	mdelay(120);
	oled_write(dev, oled_init_data[sizeof(oled_init_data)-1],1);

	data[0] = 0x00; data[1] = 0x00;data[2] = 320>>8; data[3] = 320 & 0xff; 
	oled_write_reg_data(dev, 0x2a, data, 4);
	data[0] = 0x00; data[1] = 0x00 ;data[2] = 240>>8; data[3] = 240 & 0xff; 
	oled_write_reg_data(dev, 0x2b, data, 4);//设置
	
	gpio_direction_output(dev->dc_pin, 0);
	oled_write(dev, 0x2c, 1);//write reg
	gpio_direction_output(dev->dc_pin, 1);
	for (i=0; i<320*240; i++)
	{
		oled_write(dev, 0x1f>>8, 1);//write data
		oled_write(dev, 0x1f, 1);//write data
	}
	

	return 0;
}
static int read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	int ret;
	struct oled_dev *dev = (struct oled_dev*)filp->private_data;

	ret = oled_write_data(dev);

	if (ret < 0)
		printk("[ oled ] Fail to write \n");
	return 0;
}
static int open(struct inode *inode, struct file *filp)
{
	struct oled_dev *cdev = (struct oled_dev*)container_of(inode->i_cdev, struct oled_dev, cdev);
	filp->private_data = cdev;
	return 0;
}


static int close(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations oled_chrdev_fops = {
	.owner = THIS_MODULE,
	.open = open,
	.release = close,
	.write = write,
	.read = read,
};

static int pdrv_probe(struct spi_device *spi)
{

	int ret; 
	struct oled_dev *oled;
	dev_t cur_dev;

	
	printk("[ oled ] platform driver probe\n");

	ret = alloc_chrdev_region(&devno, 0, 1, "oled_dev");
	if (ret < 0)
	{
		printk("[ oled ] Failed to alloc chardev  \n");
		return -ENOMEM;
	}
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	oled = devm_kzalloc(&spi->dev, sizeof(struct oled_dev), GFP_KERNEL);
	if (!oled)
		return -ENOMEM;
	
	cdev_init(&oled->cdev, &oled_chrdev_fops);
	
	ret = cdev_add(&oled->cdev, cur_dev, 1);
	if (ret < 0)
	{
		printk("[ oled ] Failed to add cdev\n");
		goto del_unregister;
	}

	oled->class =  class_create(THIS_MODULE, "oled_class");
	if ( !oled->class)
	{
		printk("[ oled ] Failed to create class\n");
		goto del_cdev;
	}

	oled->device = device_create(oled->class, NULL, cur_dev, NULL, "oled_dev");
	if ( !oled->device)
	{
		printk("[ oled ] Failed to create class\n");
		goto del_class;
	}

	/* spi init */
	oled->oled_node = of_find_node_by_path("/soc/spi@44009000/spi_oled@0");
	oled->dc_pin = of_get_named_gpio(oled->oled_node, "d_c_control_pin", 0);
	gpio_request(oled->dc_pin, "dc_pin");
	gpio_direction_output(oled->dc_pin, 1);
	oled->spi = spi;
	oled->spi->max_speed_hz = 2000000;
	oled->spi->mode = SPI_MODE_0;
	spi_setup(oled->spi);

	spi_set_drvdata(spi, oled);
	return 0;

del_class:
	class_destroy(oled->class);
del_cdev:
	cdev_del(&oled->cdev);
del_unregister:
	unregister_chrdev_region(cur_dev, 1);

	return ret;
}

static int pdrv_remove( struct spi_device *spi)
{
	dev_t cur_dev;
	
	struct oled_dev *oled = spi_get_drvdata(spi);
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	device_destroy(oled->class, cur_dev);
	class_destroy(oled->class);
	cdev_del(&oled->cdev);
	unregister_chrdev_region(cur_dev, 1);
	gpio_free(oled->dc_pin);
	return 0;
}

static struct spi_driver oled_driver = {
	.probe = pdrv_probe,
	.remove = pdrv_remove,
	.driver = {
		.name = "spi_oled",
		.owner = THIS_MODULE,
		.of_match_table = spi_oled,
	}
};

static __init int pdrv_init(void)
{
	return spi_register_driver(&oled_driver);

}

static __exit void pdrv_exit(void)
{
	spi_unregister_driver(&oled_driver);
}
module_init(pdrv_init);
module_exit(pdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("oled module");
MODULE_ALIAS("oled driver");


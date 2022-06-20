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

static int oled_send_one_u8(struct oled_dev *cdev, u8 data);
static int oled_send_command(struct oled_dev *cdev, u8 command);
void oled_fill(struct oled_dev *cdev, unsigned char bmp_dat);
static int oled_send_data(struct oled_dev *cdev, u8 *data, u16 lenght);

static int oled_send_one_u8(struct oled_dev *cdev, u8 data)
{
	int error = 0;
	u8 tx_data = data;
	struct spi_message *message;   //定义发送的消息
	struct spi_transfer *transfer; //定义传输结构体

	/*设置 D/C引脚为高电平*/
	gpio_direction_output(cdev->dc_pin, 1);

	/*申请空间*/
	message = kzalloc(sizeof(struct spi_message), GFP_KERNEL);
	transfer = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

	/*填充message和transfer结构体*/
	transfer->tx_buf = &tx_data;
	transfer->len = 1;
	spi_message_init(message);
	spi_message_add_tail(transfer, message);

	error = spi_sync(cdev->spi, message);
	kfree(message);
	kfree(transfer);
	if (error != 0)
	{
		printk("spi_sync error! \n");
		return -1;
	}
	return 0;
}


static int oled_send_command(struct oled_dev *cdev, u8 command)
{
	int ret = 0;
	u8 tx_data = command;
	struct spi_message *message;
	struct spi_transfer *transfer;

	message = kzalloc(sizeof(struct spi_message), GFP_KERNEL);
	transfer = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

	gpio_direction_output(cdev->dc_pin, 0);
	transfer->tx_buf = &tx_data;
	transfer->len = 1;
	spi_message_init(message);
	spi_message_add_tail(transfer, message);
	ret = spi_sync(cdev->spi, message);
	kfree(message);
	kfree(transfer);
	gpio_direction_output(cdev->dc_pin, 1);
	if(ret < 0)
	{
		printk("[ oled ] spin_sync error\n");
		return -1;
	}

	return ret;
}
void oled_fill(struct oled_dev *cdev, unsigned char bmp_dat)
{
	u8 y, x;
	for (y = 0; y < 8; y++)
	{
		oled_send_command(cdev, 0xb0 + y);
		oled_send_command(cdev, 0x01);
		oled_send_command(cdev, 0x10);
		// msleep(100);
		for (x = 0; x < 128; x++)
		{
			oled_send_one_u8(cdev, bmp_dat);
		}
	}
}
static int oled_send_data(struct oled_dev *cdev, u8 *data, u16 lenght)
{
	int error = 0;
	int index = 0;
	struct spi_message *message;   //定义发送的消息
	struct spi_transfer *transfer; //定义传输结构体

	/*设置 D/C引脚为高电平*/
	gpio_direction_output(cdev->dc_pin, 1);

	/*申请空间*/
	message = kzalloc(sizeof(struct spi_message), GFP_KERNEL);
	transfer = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

	/*每次发送 30字节，循环发送*/
	do
	{
		if (lenght > 30)
		{
			transfer->tx_buf = data + index;
			transfer->len = 30;
			spi_message_init(message);
			spi_message_add_tail(transfer, message);
			index += 30;
			lenght -= 30;
		}
		else
		{
			transfer->tx_buf = data + index;
			transfer->len = lenght;
			spi_message_init(message);
			spi_message_add_tail(transfer, message);
			index += lenght;
			lenght = 0;
		}
		error = spi_sync(cdev->spi, message);
		if (error != 0)
		{
			printk("spi_sync error! %d \n", error);
			return -1;
		}

	} while (lenght > 0);

	kfree(message);
	kfree(transfer);

	return 0;
}
static int oled_display_buffer(struct oled_dev *cdev, u8 *display_buffer, u8 x, u8 y, u16 length)
{
	u16 index = 0;
	int error = 0;

	do
	{
		/*设置写入的起始坐标*/
		error += oled_send_command(cdev, 0x2a);
		error += oled_send_command(cdev, 0x00);
		error += oled_send_command(cdev, 0x00);

		error += oled_send_command(cdev, 0x00);
		error += oled_send_command(cdev, x+0xf0);
		
		error += oled_send_command(cdev, 0x2B);
		error += oled_send_command(cdev, 0x00);
		error += oled_send_command(cdev, 0x00);
		error += oled_send_command(cdev, 0x00);
		error += oled_send_command(cdev, 0x00+0xf0);
		error += oled_send_command(cdev, 0x2c);

		error += oled_send_command(cdev, 0xff);
		error += oled_send_command(cdev, 0xff);

		//for (index=0; index < 5; index++)
		//{
			error += oled_send_command(cdev, 0x00);	
			error += oled_send_command(cdev, 0x1f);
			//mdelay(20);
		//}
		if (length > (X_WIDTH - x))
		{
		//	error += oled_send_data(cdev, display_buffer + index, X_WIDTH - x);
			length -= (X_WIDTH - x);
			index += (X_WIDTH - x);
			x = 0;
			y++;
		}
		else
		{
		//	error += oled_send_data(cdev, display_buffer + index, length);
			index += length;
			// x += length;
			length = 0;
		}

	} while (length > 0);

	if (error != 0)
	{
		/*发送错误*/
		printk("oled_display_buffer error! %d \n",error);
		return -1;
	}
	return index;
}

static int write(struct file *filp, const char __user *buf, size_t cnt, loff_t *off)
{
	int ret;
	struct oled_display *data;
	struct oled_dev *cdev = (struct oled_dev *)filp->private_data;
	data = (struct oled_display*)kzalloc(cnt, GFP_KERNEL);
	ret = copy_from_user(data, buf,cnt);

	oled_display_buffer(cdev,data->buf, data->x, data->y, data->length);
	kfree(data);
	return 0;
}
static int open(struct inode *inode, struct file *filp)
{
	int i;
	struct oled_dev *cdev = (struct oled_dev*)container_of(inode->i_cdev, struct oled_dev, cdev);
	filp->private_data = cdev;
	for (i=0; i<sizeof(oled_init_data); i++)
	{
		
		if (i == sizeof(oled_init_data)-1)
		{
			mdelay(120);
		}
		oled_send_command(cdev, oled_init_data[i]);
	}
	//oled_fill(cdev, 0x00);
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


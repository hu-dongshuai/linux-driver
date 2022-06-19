#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
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
#include "i2c_mpu6050.h"
#include <linux/platform_device.h>

static struct of_device_id mpu_6050[] = {
	{.compatible = "i2c_mpu6050"},
	{}
};

struct mpu_dev
{
	struct cdev mpu_cdev;
	struct class *class;
	struct device *device;
	struct i2c_client *client;
};
static dev_t devno;

static int i2c_write_mpu(struct i2c_client *client, u8 address, u8 data)
{
	int ret;
	struct i2c_msg send_msg;
	u8 write_data[2];
	write_data[0] = address;
	write_data[1] = data;

	send_msg.addr = client->addr;
	send_msg.flags = 0;
	send_msg.buf = write_data;
	send_msg.len = 2;

	ret = i2c_transfer(client->adapter, &send_msg, 1);
	if (ret != 1)
	{
		printk("[ mup ] transfer error\n");
		return -1;
	}
	return 0;

}
static int mpu6050_init(struct i2c_client *client)
{
	int error = 0;
	/*配置mpu6050*/
	error += i2c_write_mpu(client, PWR_MGMT_1, 0X00);
	error += i2c_write_mpu(client, SMPLRT_DIV, 0X07);
	error += i2c_write_mpu(client, CONFIG, 0X06);
	error += i2c_write_mpu(client, ACCEL_CONFIG, 0X01);

	if (error < 0)
	{
		/*初始化错误*/
		printk(KERN_DEBUG "\n mpu6050_init error \n");
		return -1;
	}
	return 0;
}
static int led_open(struct inode *inode, struct file *filp)
{
	struct mpu_dev *cdev = (struct mpu_dev*)container_of(inode->i_cdev, struct mpu_dev, mpu_cdev);
	filp->private_data = cdev;
	mpu6050_init(cdev->client);
	return 0;
}

static int i2c_read(struct i2c_client *client, u8 addr, void *data, u32 length)
{
	int ret;
	u8 address_data = addr;
	struct i2c_msg msg[2];

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = &address_data;
	msg[0].len = 1;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;
	msg[1].len = length;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2)
	{
		printk("[ mpu ] Failed to read\n");
		return -1;
	}
	return 0;
}
static ssize_t mpu_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	char data_H;
	char data_L;
	int error;
	short mpu6050_result[6]; 
	struct mpu_dev *cdev = (struct mpu_dev*)filp->private_data;
	
	i2c_read(cdev->client, ACCEL_XOUT_H, &data_H, 1);
	i2c_read(cdev->client, ACCEL_XOUT_L, &data_L, 1);
	mpu6050_result[0] = data_H << 8;
	mpu6050_result[0] += data_L;

	i2c_read(cdev->client, ACCEL_YOUT_H, &data_H, 1);
	i2c_read(cdev->client, ACCEL_YOUT_L, &data_L, 1);
	mpu6050_result[1] = data_H << 8;
    mpu6050_result[1] += data_L;

	i2c_read(cdev->client, ACCEL_ZOUT_H, &data_H, 1);
	i2c_read(cdev->client, ACCEL_ZOUT_L, &data_L, 1);
	mpu6050_result[2] = data_H << 8;
	mpu6050_result[2] += data_L;

	i2c_read(cdev->client, GYRO_XOUT_H, &data_H, 1);
	i2c_read(cdev->client, GYRO_XOUT_L, &data_L, 1);
	mpu6050_result[3] = data_H << 8;
	mpu6050_result[3] += data_L;

	i2c_read(cdev->client, GYRO_YOUT_H, &data_H, 1);
	i2c_read(cdev->client, GYRO_YOUT_L, &data_L, 1);
	mpu6050_result[4] = data_H << 8;
	mpu6050_result[4] += data_L;

	i2c_read(cdev->client, GYRO_ZOUT_H, &data_H, 1);
	i2c_read(cdev->client, GYRO_ZOUT_L, &data_L, 1);
	mpu6050_result[5] = data_H << 8;
	mpu6050_result[5] += data_L;

	/*将读取得到的数据拷贝到用户空间*/
	error = copy_to_user(buf, mpu6050_result, cnt);

	if(error != 0)
	{
		printk("copy_to_user error!");
		return -1;
	}
	return 0;
}
static int led_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations mpu_chrdev_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_close,
	.read = mpu_read,
};

static int pdrv_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

	int ret; 
	struct mpu_dev *mpu;
	dev_t cur_dev;

	
	printk("[ mpu ] platform driver probe\n");

	ret = alloc_chrdev_region(&devno, 0, 1, "led_dev");
	if (ret < 0)
	{
		printk("[ mpu ] Failed to alloc chardev  \n");
		return -ENOMEM;
	}
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	mpu = devm_kzalloc(&client->dev, sizeof(struct mpu_dev), GFP_KERNEL);
	if (!mpu)
		return -ENOMEM;
	
	mpu->client = client;
	cdev_init(&mpu->mpu_cdev, &mpu_chrdev_fops);
	
	ret = cdev_add(&mpu->mpu_cdev, cur_dev, 1);
	if (ret < 0)
	{
		printk("[ mpu ] Failed to add cdev\n");
		goto del_unregister;
	}

	mpu->class =  class_create(THIS_MODULE, "mpu_class");
	if ( !mpu->class)
	{
		printk("[ mpu ] Failed to create class\n");
		goto del_cdev;
	}

	mpu->device = device_create(mpu->class, NULL, cur_dev, NULL, "mpu_dev");
	if ( !mpu->device)
	{
		printk("[ mpu ] Failed to create class\n");
		goto del_class;
	}
	i2c_set_clientdata(client, mpu);
	return 0;

del_class:
	class_destroy(mpu->class);
del_cdev:
	cdev_del(&mpu->mpu_cdev);
del_unregister:
	unregister_chrdev_region(cur_dev, 1);

	return ret;
}

static int pdrv_remove( struct i2c_client *client)
{
	dev_t cur_dev;
	
	struct mpu_dev *mpu = i2c_get_clientdata(client);
	cur_dev = MKDEV(MAJOR(devno), MINOR(devno));
	device_destroy(mpu->class, cur_dev);
	class_destroy(mpu->class);
	cdev_del(&mpu->mpu_cdev);
	unregister_chrdev_region(cur_dev, 1);
	return 0;
}

static struct i2c_driver mpu_driver = {
	.probe = pdrv_probe,
	.remove = pdrv_remove,
	.driver = {
		.name = "mpu6050",
		.owner = THIS_MODULE,
		.of_match_table = mpu_6050,
	}
};

static __init int pdrv_init(void)
{
	i2c_add_driver(&mpu_driver);
	return 0;
}

static __exit void pdrv_exit(void)
{
	i2c_del_driver(&mpu_driver);
}
module_init(pdrv_init);
module_exit(pdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("hello module");
MODULE_ALIAS("test hello");


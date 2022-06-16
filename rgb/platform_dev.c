#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>



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

static struct resource rled_resource[] = {
	[0] = DEFINE_RES_MEM(GPIOA_PORT_MOD, 4),
	[1] = DEFINE_RES_MEM(GPIOA_OUTPUT_TYPE, 4),
	[2] = DEFINE_RES_MEM(GPIOA_OUTPUT_SPEED, 4),
	[3] = DEFINE_RES_MEM(GPIOA_PULL_UP_DOWN, 4),
	[4] = DEFINE_RES_MEM(GPIOA_PORT_SET_RESET, 4),
	[5] = DEFINE_RES_MEM(RCC_OCENSETR, 4),
};
unsigned int rled_hwinfo[2] = { 13, 0 };
static void led_release(struct device *dev)
{

}

static struct platform_device rled_pdev = {
	.name = "led_pdev",
	.id = 0,
	.num_resources = ARRAY_SIZE(rled_resource),
	.resource = rled_resource,
	.dev = {
		.release = led_release, 
		.platform_data = rled_hwinfo,
	},
};

static int __init hello_init(void)
{
	printk("[ default ] dev init");
	platform_device_register(&rled_pdev);
	return 0;
}

static void __exit hello_exit(void)
{
	platform_device_unregister(&rled_pdev);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hds");
MODULE_DESCRIPTION("hello module");
MODULE_ALIAS("test hello");


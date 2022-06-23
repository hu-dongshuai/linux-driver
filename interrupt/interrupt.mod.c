#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf6bea569, "module_layout" },
	{ 0x9c7e95b7, "platform_driver_unregister" },
	{ 0x61ed4347, "__platform_driver_register" },
	{ 0x2ce607fc, "device_create" },
	{ 0xcc374b16, "__class_create" },
	{ 0x1a843170, "cdev_add" },
	{ 0x7e8d8f84, "cdev_init" },
	{ 0xfad7a4a, "devm_kmalloc" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0xc0fd0fb6, "irq_of_parse_and_map" },
	{ 0x6fb48941, "gpiod_direction_input" },
	{ 0x243eeffb, "gpio_to_desc" },
	{ 0x70966f66, "of_get_named_gpio_flags" },
	{ 0xcb8eca16, "of_find_node_opts_by_path" },
	{ 0xc1514a3b, "free_irq" },
	{ 0xfe990052, "gpio_free" },
	{ 0x7c32d0f0, "printk" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x6fb4652e, "cdev_del" },
	{ 0x3493d078, "class_destroy" },
	{ 0x3f2312f1, "device_destroy" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


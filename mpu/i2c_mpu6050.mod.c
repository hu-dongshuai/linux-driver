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
	{ 0x89960fc2, "i2c_del_driver" },
	{ 0x3cda95b8, "i2c_register_driver" },
	{ 0x1e047854, "warn_slowpath_fmt" },
	{ 0x189c5980, "arm_copy_to_user" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0x9d3e7f4c, "i2c_transfer" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x2ce607fc, "device_create" },
	{ 0xcc374b16, "__class_create" },
	{ 0x1a843170, "cdev_add" },
	{ 0x7e8d8f84, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
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


#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
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
__used __section("__versions") = {
	{ 0xd0758a31, "proc_create" },
	{ 0x92997ed8, "_printk" },
	{ 0x5427305b, "single_open" },
	{ 0xea19ae9c, "hw1_recent_schedules" },
	{ 0x2c9de66f, "seq_printf" },
	{ 0xc60d0620, "__num_online_cpus" },
	{ 0xe2388ea9, "hw1_recent_schedules_index" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0xfb62350b, "remove_proc_entry" },
	{ 0xfb08b0a1, "seq_read" },
	{ 0x6e389bf3, "seq_lseek" },
	{ 0x55133893, "single_release" },
	{ 0xd18930e6, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "8448C6585C8E4697516C8FE");

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
	{ 0x622ce79f, "seq_open" },
	{ 0x2c9de66f, "seq_printf" },
	{ 0xfb62350b, "remove_proc_entry" },
	{ 0xfb08b0a1, "seq_read" },
	{ 0x6e389bf3, "seq_lseek" },
	{ 0xecdbc58b, "seq_release" },
	{ 0xd18930e6, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "5A5F34A596B5AFB1855814D");

/* Queste prime tre inlcude le userai praticamente sempre*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

/* Standard module information, edit as appropriate */
MODULE_LICENSE("GPL");
MODULE_AUTHOR
    ("Ivan Piri was here");
MODULE_DESCRIPTION
    ("adderdriver - loadable module template generated by petalinux-create -t modules");

static int init_hello(void)
{
	pr_info("Hello from module adderdriver\n");
	return 0;
}

static void exit_hello(void)
{
	pr_info("Goodbye from module adderdriver\n");
}

module_init(init_hello);
module_exit(exit_hello);


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

// Nome che esce in /proc/modules, /sys/bus/platform/drivers eccetera
#define DRIVER_NAME "adder"

/* Standard module information, edit as appropriate */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Piri was here");
MODULE_DESCRIPTION("adder - loadable module that interfaces with a pl adder");

struct adder_local {
	int irq;
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};

/* Simple example of how to receive command line parameters to your module.
   Delete if you don't need them */
unsigned myint = 0xdeadbeef;
char *mystr = "default";

module_param(myint, int, S_IRUGO);
module_param(mystr, charp, S_IRUGO);

// Nel caso in cui volessimo usare gli interrupt
static irqreturn_t adder_irq(int irq, void *lp)
{
	printk("adder interrupt\n");
	return IRQ_HANDLED;
}

static int adder_probe(struct platform_device *pdev)
{
	struct resource *r_irq; /* Interrupt resources */
	struct resource *r_mem; /* IO mem resources */
	struct device *dev = &pdev->dev;
	struct adder_local *lp = NULL;

	int rc = 0;
	dev_info(dev, "Device Tree Probing\n");
	/* Get iospace for the device */
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r_mem) {
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}
	lp = (struct adder_local *) kmalloc(sizeof(struct adder_local), GFP_KERNEL);
	if (!lp) {
		dev_err(dev, "Cound not allocate adder device\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, lp);
	lp->mem_start = r_mem->start;
	lp->mem_end = r_mem->end;

	if (!request_mem_region(lp->mem_start,
				lp->mem_end - lp->mem_start + 1,
				DRIVER_NAME)) {
		dev_err(dev, "Couldn't lock memory region at %p\n",
			(void *)lp->mem_start);
		rc = -EBUSY;
		goto error1;
	}

	lp->base_addr = ioremap(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	if (!lp->base_addr) {
		dev_err(dev, "adder: Could not allocate iomem\n");
		rc = -EIO;
		goto error2;
	}

	/* Get IRQ for the device */
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!r_irq) {
		dev_info(dev, "no IRQ found\n");
		dev_info(dev, "adder at 0x%08x mapped to 0x%08x\n",
			(unsigned int __force)lp->mem_start,
			(unsigned int __force)lp->base_addr);
		return 0;
	}
	lp->irq = r_irq->start;
	rc = request_irq(lp->irq, &adder_irq, 0, DRIVER_NAME, lp);
	if (rc) {
		dev_err(dev, "testmodule: Could not allocate interrupt %d.\n",
			lp->irq);
		goto error3;
	}

	dev_info(dev,"adder at 0x%08x mapped to 0x%08x, irq=%d\n",
		(unsigned int __force)lp->mem_start,
		(unsigned int __force)lp->base_addr,
		lp->irq);
	return 0;
error3:
	free_irq(lp->irq, lp);
error2:
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
error1:
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return rc;
}

static int adder_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct adder_local *lp = dev_get_drvdata(dev);
	free_irq(lp->irq, lp);
	iounmap(lp->base_addr);
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return 0;
}


static struct of_device_id adder_of_match[] = {
	{ .compatible = "xlnx,axi4-adder-1.0", },
	{},
};

/*
Each driver in the code exposes its vendor/device id using:
MODULE_DEVICE_TABLE(of, omap_mcspi_of_match);
At compilation time the build process extracts this infomation from all the drivers 
and prepares a device table.
When you insert the device, the device table is referred by the kernel and if an entry 
is found matching the device/vendor id of the added device, then its module is loaded 
and initialized.
*/
MODULE_DEVICE_TABLE(of, adder_of_match);

static struct platform_driver adder_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table	= adder_of_match,
	},
	.probe		= adder_probe,
	.remove		= adder_remove,
};


/**
 * Potresti far sparire init ed exit e sostituirle con
 * - module_platform_driver(adder_driver);
 * dentro questa macro vengono astratte automaticamente.
 * Chiaramente dovresti rimuovere anche module_init e module_exit qua sotto
*/
static int __init adder_init(void)
{
	printk("<1>Hello module world.\n");
	printk("<1>Module parameters were (0x%08x) and \"%s\"\n", myint,
	       mystr);

	return platform_driver_register(&adder_driver);
}

/* Quando poi passiamo al modulo inline la exit in teoria non serve piu' */
static void __exit adder_exit(void)
{
	platform_driver_unregister(&adder_driver);
	printk(KERN_ALERT "Goodbye module world.\n");
}


module_init(adder_init);
module_exit(adder_exit);

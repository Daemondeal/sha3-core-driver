#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>


/**
 * Standard module information, edit as appropriate
*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Piri was here");
MODULE_DESCRIPTION("ketchup-driver - TODO");

/**
 * DRIVER_NAME -> name of the driver
 * CLASS_NAME -> name of the class in /sys/class/
*/
#define DRIVER_NAME "ketchup-driver"
#define CLASS_NAME "KEKKAK_ACCELERATORS"

/**
 * Static variables used inside the driver
 * TODO: spostale in una struct statica
*/
static char ker_buf[100];
static int curr_len = 0;
static struct class *driver_class = NULL;
static dev_t first;
static struct cdev c_dev;
static int operand_a = 0;
static int operand_b = 0;
static int result = 0;
static int result_ready = 0;
static struct device *test_device;

/**
 * Functions prototypes in order to solve scope problems
*/
static int dev_open(struct inode *inod, struct file *fil);
static ssize_t dev_read(struct file *fil, char *buf, size_t len, loff_t *off);
static ssize_t dev_write(struct file *fil, const char *buf, size_t len, loff_t *off);
static int dev_release(struct inode *inod, struct file *fil);
static ssize_t write_attrs(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t read_attrs(struct device *dev, struct device_attribute *attr, char *buf);

/**
 * Gli attributi per sysfs
 * nome_attributo, r/w, callback_lettura, callback_scrittura
*/
/*
static DEVICE_ATTR(write_reg_1, S_IWUSR, NULL, write_attrs);
static DEVICE_ATTR(write_reg_2, S_IWUSR, NULL, write_attrs);
static DEVICE_ATTR(go, S_IWUSR, NULL, write_attrs);
static DEVICE_ATTR(read, S_IRUGO, read_attrs, NULL);
*/

/**
 * The various attributes that will pop up in /sys/class/CLASS_NAME/DRIVER_NAME/
 * The macro arguments are attribute_name, r/w/, read_callback, write_callback
 * Remember that each attribute will expand in dev_attr_attribute-name so add the
 * correct code in the init and exit functions
*/
static DEVICE_ATTR(test, S_IWUSR, NULL, write_attrs);

/**
 * Struct representing the character device
*/
static struct file_operations fops = {
	.read=dev_read,
	.write=dev_write,
	.open=dev_open,
	.release=dev_release,
};

static int dev_open(struct inode *inod, struct file *fil)
{
	pr_info("Device ketchup_accel opened \n");
	return 0;
}

static int dev_release(struct inode *inod, struct file *fil)
{
	pr_info("Device ketchup_accel closed \n");
	return 0;
}

static ssize_t dev_read(struct file *fil, char *buf, size_t len, loff_t *off)
{
	/*
	if (result_ready)
	{
		result_ready = 0;
		pr_info("Reading device rx: %d \n", (int)len);
		int n = sprintf(ker_buf, "%d\n", result);
		copy_to_user(buf, ker_buf, n);
		pr_info("Returning %s rx: %d\n", ker_buf, n);
		return n;
	}
	*/
	pr_info("dev_read called!\n");
	return 0;
}

static ssize_t dev_write(struct file *fil, const char *buf, size_t len, loff_t *off)
{
	/*
	// Copiamo da user space a kernel space
	copy_from_user(ker_buf, buf, len);
	sscanf(ker_buf, "%d,%d", &operand_a, &operand_b);
	ker_buf[len] = 0;
	pr_info("Received the following operands <%d + %d>", operand_a, operand_b);
	*/
	pr_info("dev_write called!\n");
	return len;
}

/**
 * Aggiungiamo le funzioni che si occupano degli attributi
*/
static ssize_t write_attrs(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	pr_info("Received %d bytes attribute %s\n", (int)count, buf);
	return count;
}


static ssize_t read_attrs(struct device *dev, struct device_attribute *attr, char *buf)
{
	sprintf(buf, "Result is: %d\n", result);
	return strlen(buf) + 1;
}

/* Simple example of how to receive command line parameters to your module.
   Delete if you don't need them */
unsigned myint = 0xdeadbeef;
char *mystr = "default";
module_param(myint, int, S_IRUGO);
module_param(mystr, charp, S_IRUGO);

/**
 * Struct representing the platform driver, maybe we can put all the static variables 
 * here?
*/
struct ketchup_driver_local {
	int irq;
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};

/**
 * In theory we don't need this
 * TODO: remove it
*/
static irqreturn_t ketchup_driver_irq(int irq, void *lp)
{
	printk("ketchup-driver interrupt\n");
	return IRQ_HANDLED;
}

/**
 * Probe function that's called after the init function when
 * the driver is loaded. 
 * It's run once for every device that will use our driver
*/
static int ketchup_driver_probe(struct platform_device *pdev)
{
	struct resource *r_irq; /* Interrupt resources */
	struct resource *r_mem; /* IO mem resources */
	struct device *dev = &pdev->dev;
	struct ketchup_driver_local *lp = NULL;

	int rc = 0;
	dev_info(dev, "Device Tree Probing\n");
	/* Get iospace for the device */
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r_mem) {
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}
	lp = (struct ketchup_driver_local *) kmalloc(sizeof(struct ketchup_driver_local), GFP_KERNEL);
	if (!lp) {
		dev_err(dev, "Cound not allocate ketchup-driver device\n");
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
		dev_err(dev, "ketchup-driver: Could not allocate iomem\n");
		rc = -EIO;
		goto error2;
	}

	/* Get IRQ for the device */
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!r_irq) {
		dev_info(dev, "no IRQ found\n");
		dev_info(dev, "ketchup-driver at 0x%08x mapped to 0x%08x\n",
			(unsigned int __force)lp->mem_start,
			(unsigned int __force)lp->base_addr);

		// Configuro i puntatori ai registri
		/*
		regA = (unsigned int __force)lp->base_addr;
		regB = (unsigned int __force)lp->base_addr + 0x04;
		regC = (unsigned int __force)lp->base_addr + 0x08;
		pr_info("Registro A; 0x%08x\n", (unsigned int)regA);
		pr_info("Registro B; 0x%08x\n", (unsigned int)regB);
		pr_info("Registro C; 0x%08x\n", (unsigned int)regC);
		*/
		return 0;
	}
	lp->irq = r_irq->start;
	rc = request_irq(lp->irq, &ketchup_driver_irq, 0, DRIVER_NAME, lp);
	if (rc) {
		dev_err(dev, "testmodule: Could not allocate interrupt %d.\n",
			lp->irq);
		goto error3;
	}

	dev_info(dev,"ketchup-driver at 0x%08x mapped to 0x%08x, irq=%d\n",
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

/**
 * Function called when the "rmmod ketchup-driver" command is given
 * immediatly after the exit function
 * It's called once for every device registered to our driver
*/
static int ketchup_driver_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ketchup_driver_local *lp = dev_get_drvdata(dev);
	free_irq(lp->irq, lp);
	iounmap(lp->base_addr);
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return 0;
}

/**
 * TODO: update with the right compatible string
*/
static struct of_device_id ketchup_driver_of_match[] = {
	{ .compatible = "xlnx,KetchupPeripheralParametrized-1.0", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, ketchup_driver_of_match);

/**
 * Struct representing all the platform driver informations
*/
static struct platform_driver ketchup_driver_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table	= ketchup_driver_of_match,
	},
	.probe		= ketchup_driver_probe,
	.remove		= ketchup_driver_remove,
};

/**
 * init function, it's the first one that is called when the driver is loaded
*/
static int __init ketchup_driver_init(void)
{
	printk("<1>Hello module world.\n");
	printk("<1>Module parameters were (0x%08x) and \"%s\"\n", myint,
	       mystr);
	/**
	 * TODO: devi capire a cosa serve il quarto argomento
	*/
	if (alloc_chrdev_region(&first, 0, 1, "Lorenzo") < 0)
	{
		return -1;
	}

	driver_class = class_create(THIS_MODULE, CLASS_NAME);
	if (driver_class == NULL)
	{
		pr_info("Create class failed \n");
		unregister_chrdev_region(first, 1);
		return -1;
	}

	test_device = device_create(driver_class, NULL, first, NULL, "ketchup_driver");
	if (test_device == NULL)
	{
		pr_info("Create device failed \n");
		class_destroy(driver_class);
		unregister_chrdev_region(first, 1);
		return -1;
	}

	cdev_init(&c_dev, &fops);

	if (cdev_add(&c_dev, first, 1) == -1)
	{
		pr_info("create character device failed\n");
		device_destroy(driver_class, first);
		class_destroy(driver_class);
		unregister_chrdev_region(first, 1);
		return -1;
	}

	if (device_create_file(test_device, &dev_attr_test) < 0)
	{
		pr_info("Device attribute creation failed\n");
	}

	return platform_driver_register(&ketchup_driver_driver);
}

/**
 * Exit function, called when we run "rmmod ketchup-driver"
*/
static void __exit ketchup_driver_exit(void)
{
	pr_info("03-exit\n");
	cdev_del(&c_dev);
	device_remove_file(test_device, &dev_attr_test);
	device_destroy(driver_class, first);
	class_destroy(driver_class);
	platform_driver_unregister(&ketchup_driver_driver);
	printk(KERN_ALERT "Goodbye module world.\n");
}

module_init(ketchup_driver_init);
module_exit(ketchup_driver_exit);

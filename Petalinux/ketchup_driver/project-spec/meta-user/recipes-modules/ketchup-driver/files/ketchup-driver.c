#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include "ketchup-driver.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Piri was here");
MODULE_DESCRIPTION("ketchup-driver - TODO");

/**
 * Struct representing the character device
*/
static struct file_operations fops = {
	.read=dev_read,
	.write=dev_write,
	.open=dev_open,
	.release=dev_release,
};

static struct of_device_id ketchup_driver_of_match[] = {
	{ .compatible = "xlnx,KetchupPeripheralParametrized-1.0", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, ketchup_driver_of_match);

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
 * Struct representing the platform driver, maybe we can put all the static variables 
 * here?
*/
struct ketchup_driver_local {
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
	void __iomem *control;
	void __iomem *status;
	void __iomem *input;
	void __iomem *command;
	void __iomem *output_base;
	// un buffer per ogni "istanza" del driver
	char ker_buf[1024];
};

static struct char_dev {
    struct class *driver_class;
    dev_t first;
    struct cdev c_dev;
    struct device *test_device;
    /**
    * Array containing a pointer to the ketchup_driver_local strcut of each peripheral
    */
    struct ketchup_driver_local all_registered_peripherals[NUM_INSTANCES];
	int count;
} my_device = {
	.driver_class = NULL,
	.count = 0
};


/**
 * The various attributes that will pop up in /sys/class/CLASS_NAME/DRIVER_NAME/
 * The macro arguments are attribute_name, r/w, read_callback, write_callback
 * Remember that each attribute will expand in dev_attr_attribute-name so add the
 * correct code in the init and exit functions.
 * I'm setting the permissions in numeric mode. 
 * Remember the order: owner, group, other users. The possible values are:
 * r (read): 4
 * w (write): 2
 * x (execute): 1
*/
// Read only
static DEVICE_ATTR(control, 0444, read_control, NULL);

static ssize_t read_control(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint32_t mask = 0x30;  // 0x110000
	int len = 0;
	for (int index = 0; index < NUM_INSTANCES; index++){
		uint32_t control_register_value = readl(my_device.all_registered_peripherals[index].control);
		uint32_t extracted_bits = (control_register_value & mask) >> 4;
		pr_info("read_control called on the peripheral %d\n", index);
		// 00 for 512, 01 for 384, 10 for 256 and 11 for 224
		const char *message;
		switch (extracted_bits)
		{
		case 0:
			/* code */
			message = "Hash size: 512\n";
			break;
		case 1:
			message = "Hash size: 384\n";
			break;
		case 2:
			message = "Hash size: 256\n";
			break;
		case 3:
			message = "Hash size: 224\n";
			break; 
		default:
			message = "Error! \n";
			break;
		}
		char index_str[2];
        snprintf(index_str, sizeof(index_str), "%d", index);
        strcat(buf, "Periferica ");
        strcat(buf, index_str);
        strcat(buf, ": ");
        strcat(buf, message);
	}
	return strlen(buf);
}

// Writable, it's always zero, OTHERS_WRITABLE? BAD IDEA - kernel
static DEVICE_ATTR(command, 0220, NULL, write_command);
static ssize_t write_command(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int peripheral;
	int reset;
	struct ketchup_driver_local selected_peripheral;
	sscanf(buf, "%d,%d", &peripheral, &reset);
	pr_info("peripheral: %d\n", peripheral);
	pr_info("reset: %d\n", reset);
	if (peripheral < 0 || peripheral > NUM_INSTANCES - 1)
	{
		pr_err("The peripheral number specified is invalid!\n");
		return strlen(buf);
	}
	if (reset != 1)
	{
		pr_err("Invalid command, pass 1 to reset the selected peripheral\n");
		return strlen(buf);
	}
	// At this point we have a valid index and a correct command
	writel((uint32_t)1, my_device.all_registered_peripherals[peripheral].command);
	pr_info("Peripheral number %d reset completed\n", peripheral);
	return strlen(buf);
}

static int dev_open(struct inode *inod, struct file *fil)
{
	#ifdef DEBUG
	pr_info("Device ketchup_accel opened \n");
	#endif
	return 0;
}

static int dev_release(struct inode *inod, struct file *fil)
{
	#ifdef DEBUG
	pr_info("Device ketchup_accel closed \n");
	#endif
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
	#ifdef DEBUG
	pr_info("dev_read called!\n");
	#endif
	pr_info("Printing all the available peripheral informations \n");
	for (int i = 0; i < my_device.count; i++){
		pr_info("Peripheral number: %d\n", i);
		pr_info("BASE_ADDRESS: 0x%08x\n", my_device.all_registered_peripherals[i].base_addr);
		pr_info("CONTROL_ADDRESS: 0x%08x\n",  my_device.all_registered_peripherals[i].control);
		pr_info("STATUS_ADDRESS: 0x%08x\n",  my_device.all_registered_peripherals[i].status);
		pr_info("INPUT_ADDRESS: 0x%08x\n",  my_device.all_registered_peripherals[i].input);
		pr_info("COMMAND_ADDRESS: 0x%08x\n",  my_device.all_registered_peripherals[i].command);	
		pr_info("OUTPUT_BASE_ADDRESS: 0x%08x\n",  my_device.all_registered_peripherals[i].output_base);
	}
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
	#ifdef DEBUG
	pr_info("dev_write called!\n");
	#endif
	return len;
}

/**
 * Probe function that's called after the init function when
 * the driver is loaded. 
 * It's run once for every device that will use our driver
*/
static int ketchup_driver_probe(struct platform_device *pdev)
{
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

	/**
	 * Setting all the base addresses of the peripheral
	*/
	lp->control = lp->base_addr + 4;
	lp->status = lp->base_addr + 8;
	lp->input = lp->base_addr + 12;
	lp->command = lp->base_addr + 16;
	lp->output_base = lp->base_addr + 20;

	#ifdef DEBUG
	pr_info("ketchup_peripheral\n");
	pr_info("BASE_ADDRESS: 0x%08x\n", lp->base_addr);
	pr_info("CONTROL_ADDRESS: 0x%08x\n", lp->control);
	pr_info("STATUS_ADDRESS: 0x%08x\n", lp->status);
	pr_info("INPUT_ADDRESS: 0x%08x\n", lp->input);
	pr_info("COMMAND_ADDRESS: 0x%08x\n", lp->command);	
	pr_info("OUTPUT_BASE_ADDRESS: 0x%08x\n", lp->output_base);
	#endif
	
	/**
	 * Before returning, we need to add the peripheral to the list of available
	 * peripherals
	*/
	my_device.all_registered_peripherals[my_device.count] = *lp;
	my_device.count++;
	pr_info("Added new peripheral to the array!\n");

	return 0;
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
	iounmap(lp->base_addr);
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return 0;
}

static char *kekkac_accel_devnode(struct device *dev, umode_t *mode)
{
        if (!mode)
                return NULL;
		pr_info("Setting the right file permissions");
        *mode = 0666;
		/**
		 * Beware, mode may be NULL when the device gets destroyed.
		*/
        return NULL;
}

/**
 * init function, it's the first one that is called when the driver is loaded
*/
static int __init ketchup_driver_init(void)
{
	#ifdef DEBUG
	pr_info("ketchup_driver_init called!\n");
	#endif
	/**
	 * The parameters are:
	 * - output parameter for the first assigned number
	 * - first of the requested range of minor numbers
	 * - the number of minor numbers required
	 * - the name of the associated device or driver
	*/
	if (alloc_chrdev_region(&my_device.first, 0, 1, DRIVER_NAME) < 0)
	{
		return -1;
	}

	my_device.driver_class = class_create(THIS_MODULE, CLASS_NAME);
	if (my_device.driver_class == NULL)
	{
		pr_info("Create class failed \n");
		unregister_chrdev_region(my_device.first, 1);
		return -1;
	}

	/**
	 * Setting the correct file permissions
	*/
	my_device.driver_class->devnode = kekkac_accel_devnode;

	my_device.test_device = device_create(my_device.driver_class, NULL, my_device.first, NULL, "ketchup_driver");
	if (my_device.test_device == NULL)
	{
		pr_info("Create device failed \n");
		class_destroy(my_device.driver_class);
		unregister_chrdev_region(my_device.first, 1);
		return -1;
	}

	cdev_init(&my_device.c_dev, &fops);

	if (cdev_add(&my_device.c_dev, my_device.first, 1) == -1)
	{
		pr_info("create character device failed\n");
		device_destroy(my_device.driver_class, my_device.first);
		class_destroy(my_device.driver_class);
		unregister_chrdev_region(my_device.first, 1);
		return -1;
	}

	// control, command
	if (device_create_file(my_device.test_device, &dev_attr_control) < 0)
	{
		pr_info("Device attribute control creation failed\n");
	}

	if (device_create_file(my_device.test_device, &dev_attr_command) < 0)
	{
		pr_info("Device attribute command creation failed\n");
	}

	return platform_driver_register(&ketchup_driver_driver);
}

/**
 * Exit function, called when we run "rmmod ketchup-driver"
*/
static void __exit ketchup_driver_exit(void)
{
	cdev_del(&my_device.c_dev);
	device_remove_file(my_device.test_device, &dev_attr_control);
	device_remove_file(my_device.test_device, &dev_attr_command);
	device_destroy(my_device.driver_class, my_device.first);
	class_destroy(my_device.driver_class);
	platform_driver_unregister(&ketchup_driver_driver);
	printk(KERN_ALERT "Goodbye module world.\n");
}

module_init(ketchup_driver_init);
module_exit(ketchup_driver_exit);

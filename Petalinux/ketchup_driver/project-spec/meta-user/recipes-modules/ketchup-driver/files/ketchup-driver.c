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
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
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
	.unlocked_ioctl=kekkac_ioctl,
	.release=dev_release,
};

static struct of_device_id ketchup_driver_of_match[] = {
	{ .compatible = "xlnx,KetchupPeripheral-1.0", },
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
 * Struct representing the platform driver
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
	char input_ker_buf[BUF_SIZE];
	char tmp_ker_buf[3];
	int num_bytes_tmp_ker_buf;
	Availability peripheral_available;
	Hash_size setted_hash_size;
};

static struct char_dev {
    struct class *driver_class;
    dev_t device;
    struct cdev c_dev;
    struct device *test_device;
    /**
    * Array containing a pointer to the ketchup_driver_local strcut of each peripheral
    */
    struct ketchup_driver_local all_registered_peripherals[NUM_INSTANCES];
	// The number of the peripherals registered
	int count;
	struct mutex lock;
} my_device = {
	.driver_class = NULL,
	.count = 0
};


/**
 * #define "ioctl name" __IOX("magic number","command number","argument type")
 * where IOX can be :
 * - “IO“: an ioctl with no parameters
 * - “IOW“: an ioctl with write parameters (copy_from_user)
 * - “IOR“: an ioctl with read parameters (copy_to_user)
 * - “IOWR“: an ioctl with both write and read parameters
 * The Magic Number is a unique number or character that will differentiate our set of ioctl calls 
 * from the other ioctl calls. some times the major number for the device is used here.
 * Command Number is the number that is assigned to the ioctl. This is used to differentiate the 
 * commands from one another.
 * The last is the type of data.
 * #define WR_SIZE _IOW(MAJOR(my_device.device), 1, uint32_t*)
#define RD_SIZE _IOR(MAJOR(my_device.device), 2, uint32_t*)
*/
#define WR_PERIPH_HASH_SIZE _IOW('ketchup', 1, uint32_t*)
#define RD_PERIPH_HASH_SIZE _IOR('ketchup', 2, uint32_t*)
static long kekkac_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	uint32_t command;
	int index_of_assigned_peripheral = (int)(uintptr_t)filp->private_data;
		switch (cmd)
	{
	case WR_PERIPH_HASH_SIZE:
		// The command is the hash size we want to write
		if (copy_from_user(&command, (int32_t*) arg, sizeof(command)))
		{
			pr_err("kekkac_ioctl: error copying the command\n");
			return -1;
		}
		if (command < 0 || command > 3)
		{
			pr_err("kekkac_ioctl: invalid hash size!\n");
			return -EINVAL;
		}
		my_device.all_registered_peripherals[index_of_assigned_peripheral].setted_hash_size = command;
		command = command << 4;
		writel(command, my_device.all_registered_peripherals[index_of_assigned_peripheral].control);
		break;
	case RD_PERIPH_HASH_SIZE:
		// We want to return the value of the hash size
		command = my_device.all_registered_peripherals[index_of_assigned_peripheral].setted_hash_size;
		if (copy_to_user((uint32_t *)arg, &command, sizeof(command)))
		{
			pr_err("keccak_ioctl: error copying data to user space");
			return -1;
		}
		break;
	default:
		pr_info("default\n");
		return -EINVAL;
	}
	return 0;
}


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
	for (int index = 0; index < my_device.count; index++){
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

/**
 * TODO: implementation, we want pid -> peripheral in use
*/
static DEVICE_ATTR(current_usage, 0444, read_current_usage, NULL);
static ssize_t read_current_usage(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_info("read_current_usage called! \n");
	return strlen(buf);
}

static int dev_open(struct inode *inod, struct file *fil)
{
	#ifdef DEBUG
	pr_info("Device ketchup_accel opened \n");
	#endif
	/**
	 * When a new file descriptor is opened we need to assign to it an available
	 * peripheral
	 * If all the peripherals are busy, we return -EBUSY
	*/
	int index_of_assigned_peripheral = peripheral_array_access(fil);
	if (index_of_assigned_peripheral == -EBUSY)
		return -EBUSY;
	return 0;
}

static int dev_release(struct inode *inod, struct file *fil)
{
	#ifdef DEBUG
	pr_info("Device ketchup_accel closed \n");
	#endif
	int op_result = peripheral_release(fil);
	if (!op_result)
		return -EAGAIN;
	return 0;
}

static ssize_t dev_read(struct file *filep, char *buf, size_t len, loff_t *off)
{
	#ifdef DEBUG
	pr_info("dev_read called!\n");
	#endif
	/**
	 * Now we handle the read. When the user reads we need to:
	 * 1. Set this write as the last one
	 * 2. Copy data from the tmp_ker_buf into the input register 
	 * 3. Start polling the peripheral on the status register
	 * 4. Once the output is ready, buffer it and return it to the user
	 * 5. Reset the peripheral
	 * 6. Set the same hash size as before in control
	*/
	// Let's set this as the last write, we need to craft a specific value
	uint32_t control_register_value = 0;
	uint32_t tmp;
	int index_of_assigned_peripheral = (int)(uintptr_t)filep->private_data;
	char buffer[4] = {0};
	int num_of_output_regs = my_device.all_registered_peripherals[index_of_assigned_peripheral].setted_hash_size/32;
	uint8_t output_buffer[512/8] = {0};
	uint32_t output_packed[512/32] = {0};
	/**
	 * bits [5:4] must be the same as the ones in setted_hash_size
	 * bit [2] must be 1
	 * bits [1:0] must be equal to num_bytes_tmp_ker_buf
	 * The following is a sketchy method  but in theory it should work. 
	*/
	tmp = my_device.all_registered_peripherals[index_of_assigned_peripheral].setted_hash_size;
	pr_info("dev_read: tmp_value = %u\n", tmp);
	control_register_value |= tmp << 4;
	// The bit [2] must be 1
	tmp = 1;
	control_register_value |= tmp << 2;
	// We need to set the bits [1:0] with the number of bytes
	tmp = my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf;
	control_register_value |= tmp;
	// We can now write into the control register
	writel(control_register_value, my_device.all_registered_peripherals[index_of_assigned_peripheral].control);
	// We need to check if there is some residual data to copy
	if (my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf > 0)
	{
		// We have bytes to copy
		memcpy(buffer, my_device.all_registered_peripherals[index_of_assigned_peripheral].tmp_ker_buf,
			my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf);
	} 
	// we can now send data to the input register
	write_into_input_reg(buffer, sizeof(buffer), index_of_assigned_peripheral);

	// Now we need to poll the peripheral, is it possible that some other bits flip? I'm supposing to have a constant 0
	while (!readl(my_device.all_registered_peripherals[index_of_assigned_peripheral].status));
	
	// When we get here we have the output, we now need to loop depending from the hash size
	for (uint32_t i = 0; i < num_of_output_regs; i++)
	{
		uint32_t value = readl(my_device.all_registered_peripherals[index_of_assigned_peripheral].output_base + 4*i);
		output_buffer[i*4 + 0] = (value >> 24) & 0xFF;
		output_buffer[i*4 + 1] = (value >> 16) & 0xFF;
		output_buffer[i*4 + 2] = (value >> 8) & 0xFF;
		output_buffer[i*4 + 3] = value & 0xFF;
	}

	copy_to_user(buf, output_buffer, sizeof(output_buffer));
	// Resetting the peripheral
	writel((uint32_t)1, my_device.all_registered_peripherals[index_of_assigned_peripheral].command);
	// Setting the same hash_size as the one just concluded
	tmp = my_device.all_registered_peripherals[index_of_assigned_peripheral].setted_hash_size;
	control_register_value = 0;
	control_register_value |= tmp << 4;
	writel(control_register_value, my_device.all_registered_peripherals[index_of_assigned_peripheral].control);

	return sizeof(output_buffer);
}

void write_into_input_reg(char temporary_buffer[], size_t buffer_size, int index)
{
	uint32_t final_value = 0;
	uint32_t tmp;
	int byte_alignment = 4;
	for (int i = 0; i < byte_alignment; i++)
	{
		tmp = temporary_buffer[i];
		final_value |= tmp << ((3-i)*8); 
	}
	writel(final_value, my_device.all_registered_peripherals[index].input);	
	// we clean also clean the temporary buffer
	memset(temporary_buffer, 0, buffer_size);
}

static ssize_t dev_write(struct file *filep, const char *buf, size_t len, loff_t *off)
{
	#ifdef DEBUG
	pr_info("dev_write called!\n");
	#endif
	/**
	 * The plan is the following:
	 * 1. we copy inside kernel space the data the user wants to write, keeping in mind that the
	 * internal buffer is limited to BUF_SIZE
	 * 2. once we have the new buf data in kernel space we start doing copy-pasting if there's previous
	 * data
	 * 3. once we are over with it we handle residual data (if present)
	*/
	// We need to retrieve from the file descriptor the peripheral index assigned
	int index_of_assigned_peripheral = (int)(uintptr_t)filep->private_data;
	pr_info("Assigned peripheral given the file descriptor: %d\n", index_of_assigned_peripheral);
	// This variable is needed at the end to know how many bytes we copied
	int internal_buffer_too_small = 0;
	// I'll use this buffer to store the data before the writel in order to facilitate the copy-paste from buffers
	char tmp_buffer[4];
	int byte_alignment = 4;
	int offset = 0;
	int remaining_bytes_to_be_sent = 0;
	// This variable will be used in case the tmp_ker_buff will be necessary
	int remaining_bytes = 0;
	// Now we want to bring inside kernel space the new write data, but how much data the user is passing?
	if (len > BUF_SIZE)
	{
		// in this case we need to truncate, we can't copy it all in one read
		copy_from_user(my_device.all_registered_peripherals[index_of_assigned_peripheral].input_ker_buf,
						buf, BUF_SIZE);
		internal_buffer_too_small = 1;
	} else 
	{
		// if we are here it means that we have less than BUF_SIZE to copy, we can use len
		copy_from_user(my_device.all_registered_peripherals[index_of_assigned_peripheral].input_ker_buf,
						buf, len);
	}

	/**
	 * Now inside the peripheral ker_buf buffer we have the data the user wants us to send to the peripheral
	 * We can begin the copy but first we need to handle the potential presence of data inside the tmp_ker_buf 
	*/
	if (my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf > 0)
	{
		/**
		 * If we are here we have some data left from a previous write, we can copy it without problems since
		 * the tmp_buffer size is 4 and at max the peripheral tmp_ker_buf has 3 bytes
		*/
		memcpy(tmp_buffer, my_device.all_registered_peripherals[index_of_assigned_peripheral].tmp_ker_buf,
		    my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf);
		/**
		 * After the copy inside tmp_buffer we'll still have some space empty, but how much? It depends from the 
		 * value of num_bytes_tmp_ker_buf
		*/
		int bytes_left = byte_alignment - my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf;
		// We can now completely fill the tmp_buf
		memcpy(tmp_buffer + my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf, 
				my_device.all_registered_peripherals[index_of_assigned_peripheral].input_ker_buf,
				bytes_left);
		// At this point the tmp_buf is ready to be sent to the peripheral
		write_into_input_reg(tmp_buffer, sizeof(tmp_buffer), index_of_assigned_peripheral);
		// We can clear the peripheral's tmp_ker_buf for future usage
		memset(my_device.all_registered_peripherals[index_of_assigned_peripheral].tmp_ker_buf, 0, 
				sizeof(my_device.all_registered_peripherals[index_of_assigned_peripheral].tmp_ker_buf));
	}

	/**
	 * At this point we have handled the possibility of residual data from previous writes. we can now
	 * complete the copy of all the remaining data inside input_ker_buf but we need to be careful, 
	 * we can only send 4 bytes at a time
	*/
	// we can calculate all the parameters
	int num_of_write_cycles;
	if (my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf > 0)
	{
		// if we have copied data from two buffers we need to consider an offset
		offset = my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf;
		// we can now reset the peripheral buffer data for future usage
		my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf = 0;
		remaining_bytes_to_be_sent = len - offset;
		// now we can check if all the data will be copied without problems
		num_of_write_cycles = remaining_bytes_to_be_sent / byte_alignment;
		// we can also check if we will be able to copy all the data without the usage of the tmp_ker_buf
		remaining_bytes = remaining_bytes_to_be_sent % byte_alignment;
	} else {
		// Here we don't have to consider any offset and we don't have to reset any peripheral variable
		num_of_write_cycles = len / byte_alignment;
		remaining_bytes = len % byte_alignment;
	}
	// We can now copy everything
	while (num_of_write_cycles != 0)
	{
		memcpy(tmp_buffer, my_device.all_registered_peripherals[index_of_assigned_peripheral].input_ker_buf + offset, 4);
		write_into_input_reg(tmp_buffer, sizeof(tmp_buffer), index_of_assigned_peripheral);
		offset += 4;
		num_of_write_cycles--;
	}
	/**
	 * At this point the last problem we have is: do we have some remaining data to be put inside the
	 * peripheral internal tmp_ker_buf buffer? Let's find out
	*/
	if (remaining_bytes)
	{
		// we still have remaining_bytes that must be copied inside the tmp_ker_buf buffer
		memcpy(my_device.all_registered_peripherals[index_of_assigned_peripheral].tmp_ker_buf,
		my_device.all_registered_peripherals[index_of_assigned_peripheral].input_ker_buf + offset, remaining_bytes);
		// We also need to set how many bytes are present in the tmp_ker_buf
		my_device.all_registered_peripherals[index_of_assigned_peripheral].num_bytes_tmp_ker_buf = remaining_bytes;
	}
	// Are we over?
	// Maybe we can also clean the internal buffer
	memset(my_device.all_registered_peripherals[index_of_assigned_peripheral].input_ker_buf, 0, 
		sizeof(my_device.all_registered_peripherals[index_of_assigned_peripheral].input_ker_buf));
	// Based on the the variable setted at the beginning of the function, we can return how many bytes we copied
	if (internal_buffer_too_small)
		return BUF_SIZE;
	return len;
}

int peripheral_array_access(struct file * filep)
{
	mutex_lock(&my_device.lock);
	pr_info("preso il controllo del mutex -> peripheral_array_access\n");
	// A questo punto il thread ha accesso esclusivo all'array delle periferiche
	int found = 0;
	int index_assigned = 0;
	for (int i = 0; i < my_device.count; i++)
	{
		pr_info("guardo la periferica numero %d \n", i);
		if (my_device.all_registered_peripherals[i].peripheral_available == AVAILABLE)
		{
			// Ne abbiamo trovata una, e' nostra
			pr_info("la periferica %d e' disponiible \n", i);
			my_device.all_registered_peripherals[i].peripheral_available = NOT_AVAILABLE;
			index_assigned = i;
			filep->private_data = (void *)(uintptr_t)i;
			found = 1;
			// We also must reset it
			writel((uint32_t)1, my_device.all_registered_peripherals[i].command);
			break;
		}
	}
	mutex_unlock(&my_device.lock);
	pr_info("sto per restituire, found vale %d ", found);
	// If found == 0 than all the peripherals are already assigned
	if (found == 0)
		return -EBUSY;
	else
		return index_assigned;
}

int peripheral_release(struct file * filep)
{
	mutex_lock(&my_device.lock);
	pr_info("Lock acquired -> peripheral release \n");
	int peripheral_index = (int)(uintptr_t)filep->private_data;
	my_device.all_registered_peripherals[peripheral_index].peripheral_available = AVAILABLE;
	writel((uint32_t)1, my_device.all_registered_peripherals[peripheral_index].command);
	mutex_unlock(&my_device.lock);
	return 1;
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
	 * Setting all the base addresses of the peripheral and the mutex
	*/
	lp->control = lp->base_addr + 4;
	lp->status = lp->base_addr + 8;
	lp->input = lp->base_addr + 12;
	lp->command = lp->base_addr + 16;
	lp->output_base = lp->base_addr + 20;
	lp->peripheral_available = AVAILABLE;
	lp->num_bytes_tmp_ker_buf = 0;
	// preparing the two buffers
	memset(lp->input_ker_buf, 0, sizeof(lp->input_ker_buf));
	memset(lp->tmp_ker_buf, 0, sizeof(lp->tmp_ker_buf));

	#ifdef DEBUG
	pr_info("ketchup_peripheral\n");
	pr_info("BASE_ADDRESS: 0x%08x\n", lp->base_addr);
	pr_info("CONTROL_ADDRESS: 0x%08x\n", lp->control);
	pr_info("STATUS_ADDRESS: 0x%08x\n", lp->status);
	pr_info("INPUT_ADDRESS: 0x%08x\n", lp->input);
	pr_info("COMMAND_ADDRESS: 0x%08x\n", lp->command);	
	pr_info("OUTPUT_BASE_ADDRESS: 0x%08x\n", lp->output_base);
	pr_info("PERIPHERAL STATUS: %d\n", lp->peripheral_available);
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
	if (alloc_chrdev_region(&my_device.device, 0, 1, DRIVER_NAME) < 0)
	{
		return -1;
	}

	my_device.driver_class = class_create(THIS_MODULE, CLASS_NAME);
	if (my_device.driver_class == NULL)
	{
		pr_info("Create class failed \n");
		unregister_chrdev_region(my_device.device, 1);
		return -1;
	}

	/**
	 * Setting the correct file permissions
	*/
	my_device.driver_class->devnode = kekkac_accel_devnode;

	my_device.test_device = device_create(my_device.driver_class, NULL, my_device.device, NULL, "ketchup_driver");
	if (my_device.test_device == NULL)
	{
		pr_info("Create device failed \n");
		class_destroy(my_device.driver_class);
		unregister_chrdev_region(my_device.device, 1);
		return -1;
	}

	cdev_init(&my_device.c_dev, &fops);

	if (cdev_add(&my_device.c_dev, my_device.device, 1) == -1)
	{
		pr_info("create character device failed\n");
		device_destroy(my_device.driver_class, my_device.device);
		class_destroy(my_device.driver_class);
		unregister_chrdev_region(my_device.device, 1);
		return -1;
	}

	// control, command, current_usage
	if (device_create_file(my_device.test_device, &dev_attr_control) < 0)
	{
		pr_err("Device attribute control creation failed\n");
	}

	if (device_create_file(my_device.test_device, &dev_attr_command) < 0)
	{
		pr_err("Device attribute command creation failed\n");
	}

	if (device_create_file(my_device.test_device, &dev_attr_current_usage) < 0)
	{
		pr_err("Device attribute current_usage creation failed\n");
	}

	// Initializing the mutex in for the array access
	mutex_init(&my_device.lock);

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
	device_remove_file(my_device.test_device, &dev_attr_current_usage);
	device_destroy(my_device.driver_class, my_device.device);
	class_destroy(my_device.driver_class);
	platform_driver_unregister(&ketchup_driver_driver);
	printk(KERN_ALERT "Goodbye module world.\n");
}

module_init(ketchup_driver_init);
module_exit(ketchup_driver_exit);

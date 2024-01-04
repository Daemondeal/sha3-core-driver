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
#include "ketchup-periph-drvr.h"

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
	int registered_periph_number;
	struct mutex lock;
} ketchup_drvr_data = {
	.driver_class = NULL,
	.registered_periph_number = 0
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
 * #define WR_SIZE _IOW(MAJOR(ketchup_drvr_data.device), 1, uint32_t*)
#define RD_SIZE _IOR(MAJOR(ketchup_drvr_data.device), 2, uint32_t*)
*/
#define WR_PERIPH_HASH_SIZE _IOW('ketchup', 1, uint32_t*)
#define RD_PERIPH_HASH_SIZE _IOR('ketchup', 2, uint32_t*)
static long kekkac_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	uint32_t command;
	int assigned_periph_index = (int)(uintptr_t)filp->private_data;
	struct ketchup_driver_local * curr_device = &ketchup_drvr_data.all_registered_peripherals[assigned_periph_index];
	#ifdef KECCAK_DEBUG
	pr_info("kekkac_ioctl called!");
	pr_info("kekkac_ioctl: we are working on the peripheral with index %d\n", assigned_periph_index);
	#endif
	switch (cmd) {
		case WR_PERIPH_HASH_SIZE:
			// The command is the hash size we want to write
			if (copy_from_user(&command, (int32_t*) arg, sizeof(command))) {
				pr_err("kekkac_ioctl: error copying the command\n");
				return -1;
			}
			if (command < 0 || command > 3) {
				pr_err("kekkac_ioctl: invalid hash size!\n");
				return -EINVAL;
			}
			curr_device->setted_hash_size = command;
			command = command << 4;
			writel(command, curr_device->control);
			break;
		case RD_PERIPH_HASH_SIZE:
			// We want to return the value of the hash size
			command = curr_device->setted_hash_size;
			if (copy_to_user((uint32_t *)arg, &command, sizeof(command))) {
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
	struct ketchup_driver_local * curr_device;
	for (int index = 0; index < ketchup_drvr_data.registered_periph_number; index++){
		curr_device = &ketchup_drvr_data.all_registered_peripherals[index];
		uint32_t control_register_value = readl(curr_device->control);
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

/**
 * Writable, it's always zero, OTHERS_WRITABLE? BAD IDEA - kernel
 * I'm also resetting the hash size to 512 don't ask me why, probably a bad idea since i'm changing
 * the meaning of the control register but i'm desperate
*/
static DEVICE_ATTR(command, 0220, NULL, write_command);
static ssize_t write_command(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int peripheral;
	uint32_t reset;
	struct ketchup_driver_local* curr_device;
	sscanf(buf, "%d,%d", &peripheral, &reset);
	pr_info("write_command called!\n");
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
	curr_device = &ketchup_drvr_data.all_registered_peripherals[peripheral];
	writel((uint32_t)reset, curr_device->command);
	// Let's also reset the hash size to a default 512
	//writel((uint32_t))
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

/**
 * This is the function that is called whenever a new open syscall is made with our
 * driver device file (/dev/kechtup_driver) as an argument
*/
static int dev_open(struct inode *inod, struct file *fil)
{
	/**
	 * When a new file descriptor is opened we need to search for an available 
	 * peripheral that can serve the request.
	 * If all the peripherals are already assigned, we return -EBUSY
	*/
	#ifdef KECCAK_DEBUG
	pr_info("dev_open: a new file descriptor has been opened\n");
	#endif
	int assigned_periph_index = peripheral_array_access(fil);
	#ifdef KECCAK_DEBUG
	pr_info("dev_open: the peripheral assigned to this file descriptor is the numer %d\n", assigned_periph_index);
	#endif
	if (assigned_periph_index == -EBUSY)
		return -EBUSY;
	return 0;
}

static int dev_release(struct inode *inod, struct file *fil)
{
	#ifdef KECCAK_DEBUG
	pr_info("Device ketchup_accel closed \n");
	#endif
	int op_result = peripheral_release(fil);
	if (!op_result)
		return -EAGAIN;
	return 0;
}

static ssize_t dev_read(struct file *filep, char *buf, size_t len, loff_t *off)
{
	#ifdef KECCAK_DEBUG
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
	int assigned_periph_index = (int)(uintptr_t)filep->private_data;
	struct ketchup_driver_local * curr_device = &ketchup_drvr_data.all_registered_peripherals[assigned_periph_index];
	char buffer[4] = {0};
	int num_of_output_regs = 0;
	// Variable needed to clear the peripheral at the end of the function
	uint32_t reset = 1;
	// We need to know the hash size
	switch (curr_device->setted_hash_size)
	{
		case HASH_512:
			num_of_output_regs = 512/32;
			break;
		case HASH_384:
			num_of_output_regs = 384/32;
			break;
		case HASH_256:
			num_of_output_regs = 256/32;
			break;
		case HASH_224:
			num_of_output_regs = 224/32;
			break;
		default:
			pr_info("dev_read: impossible\n");
	}
	uint8_t output_buffer[512/8] = {0};
	uint32_t output_packed[512/32] = {0};
	pr_info("dev_read: init phase completed\n");
	pr_info("dev_read: assigned_periph_index = %d\n", assigned_periph_index);
	pr_info("dev_read: num_of_output_regs =  %d\n", num_of_output_regs);
	/**
	 * bits [5:4] must be the same as the ones in setted_hash_size
	 * bit [2] must be 1
	 * bits [1:0] must be equal to num_bytes_tmp_ker_buf
	 * The following is a sketchy method  but in theory it should work. 
	*/
	tmp = curr_device->setted_hash_size;
	pr_info("dev_read: tmp_value = %u\n", tmp);
	control_register_value |= tmp << 4;
	// The bit [2] must be 1
	tmp = 1;
	control_register_value |= tmp << 2;
	// We need to set the bits [1:0] with the number of bytes
	tmp = curr_device->num_bytes_tmp_ker_buf;
	control_register_value |= tmp;
	// We can now write into the control register
	pr_info("dev_read: sending the last input command to the peripheral\n");
	writel(control_register_value, curr_device->control);
	// We need to check if there is some residual data to copy
	if (curr_device->num_bytes_tmp_ker_buf > 0)
	{
		// We have bytes to copy
		memcpy(buffer, curr_device->tmp_ker_buf, curr_device->num_bytes_tmp_ker_buf);
		pr_info("dev_read: we have some residual data in tmp_ker_buf that must be copied into the input register\n");
	} 
	// we can now send data to the input register
	pr_info("dev_read: sending data to the input register\n");
	write_into_input_reg(buffer, sizeof(buffer), assigned_periph_index);

	curr_device->num_bytes_tmp_ker_buf = 0;

	pr_info("dev_read: from now we poll the peripheral\n");
	// Now we need to poll the peripheral, is it possible that some other bits flip? I'm supposing to have a constant 0
	//while ((readl(ketchup_drvr_data.all_registered_peripherals[assigned_periph_index].status) & 1) == 0);
	int flag = 1;
	for (int i = 0; i < 100; i++) {
  		if ((readl(curr_device->status) & 1) != 0) { 
			flag = 0; 
			break; 
		}
	}
	if (flag) pr_info("polling failed");
	
	pr_info("dev_read: we can now read the output\n");
	// When we get here we have the output, we now need to loop depending from the hash size
	for (uint32_t i = 0; i < num_of_output_regs; i++)
	{
		uint32_t value = readl(curr_device->output_base + 4*i);
		pr_info("dev_read: inside the for loop we are reading value = %08x\n", value);
		output_buffer[i*4 + 0] = (value >> 24) & 0xFF;
		output_buffer[i*4 + 1] = (value >> 16) & 0xFF;
		output_buffer[i*4 + 2] = (value >> 8) & 0xFF;
		output_buffer[i*4 + 3] = value & 0xFF;
	}
	
	if(copy_to_user(buf, output_buffer, sizeof(output_buffer))){
		pr_err("dev_read: error copying the kernel buffer to user space\n");
		return -EAGAIN;
	}
	pr_info("dev_read: we can now reset the peripheral, we will return sizeof(output_buffer) = %d\n", sizeof(output_buffer));
	// Resetting the peripheral
	/**
	 * TODO: questionable way of resetting, in theory it should be ok but try with an explicit variable
	*/
	writel(reset, curr_device->command);
	/**
	 * After clearing we need to set the same hash size that was setted for the previous hash.
	 * How? We get the previous value, so 0,1,2,3
	 * We simply write that value inside control after shifting it by 4
	*/
	tmp = curr_device->setted_hash_size;
	pr_info("dev_read: tmp = %d\n", tmp);
	control_register_value = 0;
	control_register_value |= tmp << 4;
	writel(control_register_value, curr_device->control);

	return sizeof(output_buffer);
}

void write_into_input_reg(char temporary_buffer[], size_t buffer_size, int index)
{
	uint32_t final_value = 0;
	uint32_t tmp;
	int byte_alignment = 4;
	struct ketchup_driver_local * curr_device = &ketchup_drvr_data.all_registered_peripherals[index];
	pr_info("write_into_input_reg called!\n");
	for (int i = 0; i < byte_alignment; i++)
	{
		tmp = temporary_buffer[i];
		final_value |= tmp << ((3-i)*8); 
	}
	writel(final_value, curr_device->input);	
	// we clean also clean the temporary buffer
	memset(temporary_buffer, 0, buffer_size);
}

static ssize_t dev_write(struct file *filep, const char *buf, size_t len, loff_t *off)
{
	#ifdef KECCAK_DEBUG
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
	int assigned_periph_index = (int)(uintptr_t)filep->private_data;
	struct ketchup_driver_local * curr_device = &ketchup_drvr_data.all_registered_peripherals[assigned_periph_index];
	pr_info("dev_write: Assigned peripheral given the file descriptor: %d\n", assigned_periph_index);
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
	pr_info("dev_write: init phase completed\n");
	if (len > BUF_SIZE)
	{
		// in this case we need to truncate, we can't copy it all in one read
		if (copy_from_user(curr_device->input_ker_buf, buf, BUF_SIZE))
		{
			pr_err("dev_write: error copying the user buffer in kernel space\n");
			return -EAGAIN;
		}
		internal_buffer_too_small = 1;
		pr_info("dev_write: the user is asking to write too much data, we copied only 1024 bytes in kernel space \n");
	} else 
	{
		// if we are here it means that we have less than BUF_SIZE to copy, we can use len
		if(copy_from_user(curr_device->input_ker_buf, buf, len))
		{
			pr_err("dev_write: error copying the user buffer in kernel space \n");
			return -EAGAIN;
		}
		pr_info("dev_write: we copied the user buffer in kernel space\n");
	}
	pr_info("dev_write: now we are going to check the tmp_ker_buf for some old data \n");
	/**
	 * Now inside the peripheral ker_buf buffer we have the data the user wants us to send to the peripheral
	 * We can begin the copy but first we need to handle the potential presence of data inside the tmp_ker_buf 
	*/
	if (curr_device->num_bytes_tmp_ker_buf > 0)
	{
		pr_info("dev_write: we had previous data, getting ready to copy it\n");
		/**
		 * If we are here we have some data left from a previous write, we can copy it without problems since
		 * the tmp_buffer size is 4 and at max the peripheral tmp_ker_buf has 3 bytes
		*/
		memcpy(tmp_buffer, curr_device->tmp_ker_buf,curr_device->num_bytes_tmp_ker_buf);
		/**
		 * After the copy inside tmp_buffer we'll still have some space empty, but how much? It depends from the 
		 * value of num_bytes_tmp_ker_buf
		*/
		int bytes_left = byte_alignment - curr_device->num_bytes_tmp_ker_buf;
		// We can now completely fill the tmp_buf
		memcpy(tmp_buffer + curr_device->num_bytes_tmp_ker_buf, curr_device->input_ker_buf, bytes_left);
		// At this point the tmp_buf is ready to be sent to the peripheral
		write_into_input_reg(tmp_buffer, sizeof(tmp_buffer), assigned_periph_index);
		// We can clear the peripheral's tmp_ker_buf for future usage
		memset(curr_device->tmp_ker_buf, 0, sizeof(((struct ketchup_driver_local *)0)->tmp_ker_buf));
		
	}

	/**
	 * At this point we have handled the possibility of residual data from previous writes. we can now
	 * complete the copy of all the remaining data inside input_ker_buf but we need to be careful, 
	 * we can only send 4 bytes at a time
	*/
	// we can calculate all the parameters
	int num_of_write_cycles;
	if (curr_device->num_bytes_tmp_ker_buf > 0)
	{
		// if we have copied data from two buffers we need to consider an offset
		offset = curr_device->num_bytes_tmp_ker_buf;
		// we can now reset the peripheral buffer data for future usage
		curr_device->num_bytes_tmp_ker_buf = 0;
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
	pr_info("dev_write: we now copy the user data inside the peripheral!\n");
	pr_info("dev_write parameters:\n");
	pr_info("OFFSET: %d\n", offset);
	pr_info("REMAINING_BYTES_TO_BE_SENT: %d\n", remaining_bytes_to_be_sent);
	pr_info("NUM_OF_WRITE_CYCLES: %d\n", num_of_write_cycles);
	pr_info("REMAINING_BYTES: %d\n", remaining_bytes);
	pr_info("INTERNAL_BUFFER_TOO_SMALL: %d\n", internal_buffer_too_small);
	// We can now copy everything
	while (num_of_write_cycles != 0)
	{
		memcpy(tmp_buffer, curr_device->input_ker_buf + offset, 4);
		write_into_input_reg(tmp_buffer, sizeof(tmp_buffer), assigned_periph_index);
		offset += 4;
		pr_info("dev_write: first copy cycle of %d completed\n", num_of_write_cycles);
		num_of_write_cycles--;
	}
	/**
	 * At this point the last problem we have is: do we have some remaining data to be put inside the
	 * peripheral internal tmp_ker_buf buffer? Let's find out
	*/
	if (remaining_bytes)
	{
		pr_info("dev_write: we still have some bytes that must be saved into the tmp_ker_buf\n");
		// we still have remaining_bytes that must be copied inside the tmp_ker_buf buffer
		memcpy(curr_device->tmp_ker_buf, curr_device->input_ker_buf + offset, remaining_bytes);
		// We also need to set how many bytes are present in the tmp_ker_buf
		curr_device->num_bytes_tmp_ker_buf = remaining_bytes;
		pr_info("dev_write: we copied the data inside the tmp_ker_buf, we buffer now cointains %d bytes\n", remaining_bytes);
	}
	// Are we over?
	// Maybe we can also clean the internal buffer
	memset(curr_device->input_ker_buf, 0, sizeof(((struct ketchup_driver_local *)0)->input_ker_buf));
	// Based on the the variable setted at the beginning of the function, we can return how many bytes we copied
	pr_info("dev_write: we are returning internal_buffer_too_smal = %d | len = %d", internal_buffer_too_small, len);
	if (internal_buffer_too_small)
		return BUF_SIZE;
	return len;
}

/**
 * This function
 * TODO: complete
 * The value we are returning is an index of the all_registered_peripherals array at which
 * is present an available peripheral that from this moment will be assigned to the file 
 * descriptor passed as an argument
*/
int peripheral_array_access(struct file * filep)
{
	mutex_lock(&ketchup_drvr_data.lock);
	#ifdef KECCAK_DEBUG
	pr_info("peripheral_array_access: mutex acquired\n");
	#endif
	// At this point the thread has exclusive access to the driver data struct
	int found = 0;
	int index_assigned = 0;
	struct ketchup_driver_local * curr_device;
	uint32_t reset = 1;
	// Looping in search of an available peripheral
	for (int i = 0; i < ketchup_drvr_data.registered_periph_number; i++)
	{
		#ifdef KECCAK_DEBUG
		pr_info("peripheral_array_access: currently examining the peripheral at index %d\n", i);
		#endif
		curr_device = &ketchup_drvr_data.all_registered_peripherals[i];
		if (curr_device->peripheral_available == AVAILABLE)
		{
			// Once we enter here we have found a free peripheral
			curr_device->peripheral_available = NOT_AVAILABLE;
			index_assigned = i;
			filep->private_data = (void *)(uintptr_t)i;
			found = 1;
			// When we open a new peripheral we clear the output registers just in case
			writel(reset, curr_device->command);
			#ifdef KECCAK_DEBUG
			pr_info("peripheral_array_access: the peripheral at [%d] is available and is now reserved\n", i);
			#endif
			break;
		}
	}
	mutex_unlock(&ketchup_drvr_data.lock);
	#ifdef KECCAK_DEBUG
	pr_info("peripheral_array_access: lock released, were we able to find a periphera? found = %d\n", found);
	#endif
	// If found == 0 than all the peripherals are already assigned
	if (found == 0)
		return -EBUSY;
	else
		return index_assigned;
}

int peripheral_release(struct file * filep)
{
	mutex_lock(&ketchup_drvr_data.lock);
	pr_info("Lock acquired -> peripheral release \n");
	int peripheral_index = (int)(uintptr_t)filep->private_data;
	struct ketchup_driver_local * cur_dev = &ketchup_drvr_data.all_registered_peripherals[peripheral_index];
	cur_dev->peripheral_available = AVAILABLE;
	writel((uint32_t)1, cur_dev->command);
	mutex_unlock(&ketchup_drvr_data.lock);
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
	lp->control = lp->base_addr;
	lp->status = lp->base_addr + 4;
	lp->input = lp->base_addr + 8;
	lp->command = lp->base_addr + 12;
	lp->output_base = lp->base_addr + 16;
	lp->peripheral_available = AVAILABLE;
	lp->num_bytes_tmp_ker_buf = 0;
	// preparing the two buffers
	memset(lp->input_ker_buf, 0, sizeof(lp->input_ker_buf));
	memset(lp->tmp_ker_buf, 0, sizeof(lp->tmp_ker_buf));
	
	/**
	 * Before returning, we need to add the peripheral inside the array with all 
	 * the available peripherals 
	*/
	ketchup_drvr_data.all_registered_peripherals[ketchup_drvr_data.registered_periph_number] = *lp;
	ketchup_drvr_data.registered_periph_number++;
	#ifdef KECCAK_DEBUG
	pr_info("***********ketchup_peripheral probing***************\n");
	dev_info(dev, "(peripheral_physical_address)");
	pr_info("(virtual)base_address: 0x%08x\n", lp->base_addr);
	pr_info("(virtual)control_address: 0x%08x\n", lp->control);
	pr_info("(virtual)status_address: 0x%08x\n", lp->status);
	pr_info("(virtual)input_address: 0x%08x\n", lp->input);
	pr_info("(virtual)command_address: 0x%08x\n", lp->command);	
	pr_info("(virtual)output_base_address: 0x%08x\n", lp->output_base);
	pr_info("peripheral_availability: %d\n", lp->peripheral_available);
	pr_info("sizeof(input_ker_buf): %zu\n", sizeof(lp->input_ker_buf));
	pr_info("sizeof(tmp_ker_buf): %zu\n", sizeof(lp->tmp_ker_buf));
	pr_info("Successfully allocated ketchup peripheral at index %d \n", ketchup_drvr_data.registered_periph_number);
	#endif

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
	#ifdef KECCAK_DEBUG
	pr_info("ketchup_driver_init: starting...\n");
	#endif
	/**
	 * The parameters are:
	 * - output parameter for the first assigned number
	 * - first of the requested range of minor numbers
	 * - the number of minor numbers required
	 * - the name of the associated device or driver
	*/
	if (alloc_chrdev_region(&ketchup_drvr_data.device, 0, 1, DRIVER_NAME) < 0)
	{
		return -1;
	}

	ketchup_drvr_data.driver_class = class_create(THIS_MODULE, CLASS_NAME);
	if (ketchup_drvr_data.driver_class == NULL)
	{
		pr_info("Create class failed \n");
		unregister_chrdev_region(ketchup_drvr_data.device, 1);
		return -1;
	}

	/**
	 * Setting the correct file permissions
	*/
	ketchup_drvr_data.driver_class->devnode = kekkac_accel_devnode;

	ketchup_drvr_data.test_device = device_create(ketchup_drvr_data.driver_class, NULL, ketchup_drvr_data.device, NULL, "ketchup_driver");
	if (ketchup_drvr_data.test_device == NULL)
	{
		pr_info("Create device failed \n");
		class_destroy(ketchup_drvr_data.driver_class);
		unregister_chrdev_region(ketchup_drvr_data.device, 1);
		return -1;
	}

	cdev_init(&ketchup_drvr_data.c_dev, &fops);

	if (cdev_add(&ketchup_drvr_data.c_dev, ketchup_drvr_data.device, 1) == -1)
	{
		pr_info("create character device failed\n");
		device_destroy(ketchup_drvr_data.driver_class, ketchup_drvr_data.device);
		class_destroy(ketchup_drvr_data.driver_class);
		unregister_chrdev_region(ketchup_drvr_data.device, 1);
		return -1;
	}

	// control, command, current_usage
	if (device_create_file(ketchup_drvr_data.test_device, &dev_attr_control) < 0)
	{
		pr_err("ketchup_driver_init: control attribute creation failed\n");
	}

	if (device_create_file(ketchup_drvr_data.test_device, &dev_attr_command) < 0)
	{
		pr_err("ketchup_driver_init: command attribute creation failed\n");
	}

	if (device_create_file(ketchup_drvr_data.test_device, &dev_attr_current_usage) < 0)
	{
		pr_err("ketchup_driver_init: current_usage attribute creation failed\n");
	}

	// Initializing the mutex in for the array access
	mutex_init(&ketchup_drvr_data.lock);

	return platform_driver_register(&ketchup_driver_driver);
}

/**
 * Exit function, called when we run "rmmod ketchup-driver"
*/
static void __exit ketchup_driver_exit(void)
{
	cdev_del(&ketchup_drvr_data.c_dev);
	device_remove_file(ketchup_drvr_data.test_device, &dev_attr_control);
	device_remove_file(ketchup_drvr_data.test_device, &dev_attr_command);
	device_remove_file(ketchup_drvr_data.test_device, &dev_attr_current_usage);
	device_destroy(ketchup_drvr_data.driver_class, ketchup_drvr_data.device);
	class_destroy(ketchup_drvr_data.driver_class);
	platform_driver_unregister(&ketchup_driver_driver);
	#ifdef KECCAK_DEBUG
	pr_info("ketchup_driver_exit: exiting..\n");
	#endif
}

module_init(ketchup_driver_init);
module_exit(ketchup_driver_exit);
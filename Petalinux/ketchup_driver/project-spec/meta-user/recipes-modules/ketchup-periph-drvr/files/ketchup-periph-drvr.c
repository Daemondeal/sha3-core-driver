#include <asm-generic/errno-base.h>
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
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/ioctl.h>

// Concurrency Primitives
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/signal.h>

#include "ketchup-periph-drvr.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Piri was here");
MODULE_DESCRIPTION("ketchup-driver - a character device driver for the ketchup peripheral");


#ifdef KECCAK_DEBUG
	#define kc_info(...) pr_info(DRIVER_NAME ": " __VA_ARGS__)
#else
// If debug is disabled, simply do nothing
	#define kc_info(...)
#endif

#define kc_err(...) pr_err(DRIVER_NAME ": " __VA_ARGS__)

#define MAX_DEVICES 128

/**
 * Struct representing the character device
*/
static struct file_operations fops = {
	.read=ketchup_read,
	.write=ketchup_write,
	.open=ketchup_open,
	.unlocked_ioctl=ketchup_ioctl,
	.release=ketchup_release,
};

/**
 * Needed for the platform bus, thanks to the compatible string
 * the driver is loaded automatically at boot time
*/
static struct of_device_id ketchup_driver_of_match[] = {
	{ .compatible = "xlnx,KetchupPeripheral-1.0", },
	{},
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
 * Struct representing a peripheral
*/
struct ketchup_device {
	unsigned long mem_start;
	unsigned long mem_end;

	// Registers
	void __iomem *base_addr;
	void __iomem *control;
	void __iomem *status;
	void __iomem *input;
	void __iomem *command;
	void __iomem *output_base;

	char data_to_send[4];
	int data_to_send_length;

	Availability peripheral_available;
	pid_t current_process;
	Hash_size setted_hash_size;
};

/**
 * This is a collection of all registered peripherals
*/ 
struct ketchup_devices_container {
	struct mutex array_write_lock;
	struct semaphore dev_free_sema;

	struct ketchup_device *registered_devices[MAX_DEVICES];
	size_t registered_devices_len;
};

// This struct hold the global data
// needed for the driver
static struct char_dev {
    struct class *driver_class;
    dev_t device;
    struct cdev c_dev;
    struct device *test_device;

	struct ketchup_devices_container devices;
} ketchup_drvr_data = {
	.driver_class = NULL,
};


// ======================= Character Device ========================

/**
 * Completely clears all the internal state of a given peripheral
*/
static void peripheral_clear(struct ketchup_device *device) {
	device->peripheral_available = AVAILABLE;
	device->current_process = 0;
	device->data_to_send_length = 0;
	device->setted_hash_size = HASH_512;

	writel(1, device->command);
}

/**
 * This function is called only by dev_open.
 * The main idea is that when a new file descriptor is opened the driver has to find and
 * available peripheral so that we can uniquely assign to each open file descriptor a unique
 * peripheral (if there's one available).
 * The value we are returning is an index of the registered_devices array at which
 * is present an available peripheral that from this moment will be assigned to the file 
 * descriptor passed as an argument
*/
static int peripheral_acquire(int should_block) 
{
	int i, found, assigned_index;
	int down_retval, is_sig;
	struct ketchup_device *curr_device;
	struct ketchup_devices_container *container = &ketchup_drvr_data.devices;

	pid_t pid = task_pid_nr(current);

	kc_info("[peripheral_acquire] task %d trying to acquire peripheral (blocking = %d)\n", pid, should_block);

	if (should_block) {
		kc_info("[peripheral_acquire] task %d before locking\n", pid);
		// Block if all peripherals are taken
		down_retval = down_interruptible(&container->dev_free_sema);
		kc_info("[peripheral_acquire] task %d got unblocked. down_retval = %d\n", pid, down_retval);

		// We should immediately return if it was a signal that unblocked us
        for (i = 0; i < _NSIG_WORDS && !is_sig; i++)  {
            is_sig = current->pending.signal.sig[i] & ~current->blocked.sig[i]; 
        }

        if (is_sig || down_retval != 0) {
			kc_err(
				"[peripheral_acquire] something interrupted task %d. is_sig = %d, down_retval = %d",
				pid, is_sig, down_retval
			);

            return -EINTR;
        }
	} else {
		down_retval = down_trylock(&container->dev_free_sema);

		if (down_retval != 0) {
			// Not enough available peripherals
			return -EAGAIN;
		}
	}

	mutex_lock(&container->array_write_lock);
	kc_info("[peripheral_acquire] task %d acquired lock. Searching for free peripheral\n", pid);

	found = 0;
	for (i = 0; i < container->registered_devices_len; i++) {
		curr_device = container->registered_devices[i];

		if (curr_device->peripheral_available == AVAILABLE) {
			// We found a suitable peripheral

			found = 1;
			assigned_index = i;

			// First clear all previous state
			peripheral_clear(curr_device);

			// Make sure no other process can take this peripheral
			curr_device->peripheral_available = NOT_AVAILABLE;

			// Assign peripheral to owner process
			curr_device->current_process = task_pid_nr(current);
			
			// Set the default hash size
			curr_device->setted_hash_size = HASH_512;
			
			kc_info("[peripheral_acquire] assigned device %d to process %d.\n", i, task_pid_nr(current));
			break;
		}
	}

	mutex_unlock(&container->array_write_lock);

	if (found) {
		kc_info("[peripheral_acquire] task %d got device %d\n", pid, assigned_index);
		return assigned_index;
	} else {
		kc_err("[assign_peripheral] could not assign peripheral to process %d past the lock.\n", task_pid_nr(current));
		return -ENODEV;
	}
}

static void peripheral_release(int index)
{
	struct ketchup_device *curr_device;
	struct ketchup_devices_container *container;

	container = &ketchup_drvr_data.devices;
	mutex_lock(&container->array_write_lock);

	kc_info("[peripheral_release] releasing device %d\n", index);
	curr_device = container->registered_devices[index];

	peripheral_clear(curr_device);

	mutex_unlock(&container->array_write_lock);
	up(&container->dev_free_sema);
}

static inline struct ketchup_device *kc_get_device(struct file *filep) {
	int assigned_periph_index = (int)(uintptr_t)filep->private_data;
	return ketchup_drvr_data.devices.registered_devices[assigned_periph_index];
}

/**
 * This is the function that is called whenever a new open syscall is made with our
 * driver device file (/dev/kechtup_driver) as an argument.
 * We are also saving the pid of the process currently opening the file inside an 
 * array for the current_usage attribute
*/
static int ketchup_open(struct inode *inod, struct file *fil)
{
	/**
	 * When a new file descriptor is opened we need to search for an available 
	 * peripheral that can serve the request.
	 * If all the peripherals are already assigned, we return -EAGAIN
	*/
	struct ketchup_device *curr_device;
	int should_block = (fil->f_flags & O_NONBLOCK) == 0;

	int assigned_peripheral = peripheral_acquire(should_block);

	if (assigned_peripheral < 0) {
		return assigned_peripheral;
	}


	// Save index for read and write
	fil->private_data = (void*)(uintptr_t)assigned_peripheral;

	return 0;
}



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
*/
#define WR_PERIPH_HASH_SIZE _IOW(0xFC, 1, uint32_t*)
#define RD_PERIPH_HASH_SIZE _IOR(0xFC, 2, uint32_t*)

static long ketchup_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	uint32_t command;
	int assigned_periph_index = (int)(uintptr_t)filp->private_data;
	struct ketchup_device *curr_device = kc_get_device(filp);
	
	kc_info("[kekkac_ioctl] called!\n");
	kc_info("[kekkac_ioctl] we are working on the peripheral with index %d\n", assigned_periph_index);

	switch (cmd) {
		case WR_PERIPH_HASH_SIZE:
			// The command is the hash size we want to write
			if (copy_from_user(&command, (int32_t*) arg, sizeof(command))) {
				kc_err("[kekkac_ioctl] error copying the command\n");
				return -1;
			}
			if (command < 0 || command > 3) {
				kc_err("[kekkac_ioctl] invalid hash size!\n");
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
				kc_err("[keccak_ioctl] error copying data to user space");
				return -1;
			}
			break;
		default:
			kc_err("[keccak_ioctl] we shouldn't be here\n");
			return -EINVAL;
	}
	return 0;
}

#define kc_min(x, y) ((x) < (y) ? (x) : (y))

static inline uint32_t pack_to_u32_big_endian(uint8_t const data[4]) {
	return ((uint32_t)data[0] << 24)
		 | ((uint32_t)data[1] << 16)
		 | ((uint32_t)data[2] <<  8)
		 | ((uint32_t)data[3]);
}

static ssize_t ketchup_write(struct file *filep, const char *user_buffer, size_t user_length, loff_t *off)
{
	/**
	 * The plan is the following:
	 * 1. we copy inside kernel space the data the user wants to write, keeping in mind that the
	 * internal buffer is limited to KC_BUF_SIZE
	 * 2. once we have the new buf data in kernel space we start doing copy-pasting if there's previous
	 * data
	 * 3. once we are over with it we handle residual data (if present)
	*/
	// We need to retrieve from the file descriptor the peripheral index assigned
	struct ketchup_device *curr_device = kc_get_device(filep);
	uint8_t buffer[KC_BUF_SIZE];
	size_t buffer_length, buffer_position;
	int error;
	int peripheral_index = (int)(uintptr_t)filep->private_data;

	kc_info(
		"[ketchup_write] beginning write for peripheral %d task %d. hash_size = %d\n",
		peripheral_index, current->pid, curr_device->setted_hash_size
	);

	// 1. Copy from user space to kernel space the data to write
	buffer_length = kc_min(KC_BUF_SIZE, user_length);
	error = copy_from_user(buffer, user_buffer, buffer_length);
	if (error != 0) {
		kc_err("[ketchup_write] coudln't copy data from user. retval = %d\n", error);
		return -1;
	}
	kc_info("[ketcuhp_write] copied %d bytes from userspace", buffer_length);

	// 2. Send to peripheral in chunks of 4
	buffer_position = 0;
	while (buffer_position < buffer_length) {
		curr_device->data_to_send[curr_device->data_to_send_length] = buffer[buffer_position];
		curr_device->data_to_send_length++;
		buffer_position++;

		if (curr_device->data_to_send_length == 4) {
			uint32_t packed = pack_to_u32_big_endian(curr_device->data_to_send);
			kc_info("[ketchup_write] writing %08x to input\n", packed);
			writel(packed, curr_device->input);
			curr_device->data_to_send_length = 0;
		}
	}

	// All done
	return buffer_length;
}

/**
 * This function is the one responsible for handling all the read operations performed
 * on the /dev/ketchup_driver file.
 * When the user reads we need to:
 * 1. Set this write as the last one
 * 2. Copy data from the tmp_ker_buf into the input register
 * 3. Start polling the peripheral on the status register
 * 4. Once the output is ready, buffer it and return it to the user
 * 5. Reset the peripheral
 * 6. Set the same hash size as before in control
*/
static ssize_t ketchup_read(struct file *filep, char *user_buffer, size_t user_len, loff_t *off)
{
	int assigned_periph_index = (int)(uintptr_t)filep->private_data;
	struct ketchup_device *curr_device = kc_get_device(filep);
	uint32_t control_value = 0, packed_input, output;
	size_t hash_size_bytes, data_to_copy;
	uint8_t output_buffer[512/8];
	int error;

	// First of all, check that user requested the correct amount of bytes
	switch (curr_device->setted_hash_size)
	{
		case HASH_512:
			hash_size_bytes = 512/8;
			break;
		case HASH_384:
			hash_size_bytes = 384/8;
			break;
		case HASH_256:
			hash_size_bytes = 256/8;
			break;
		case HASH_224:
			hash_size_bytes = 224/8;
			break;
		default:
			kc_err(
				"[ketchup_read] the peripheral %d owned by task %d has an impossible hash size (%d)!\n", 
				assigned_periph_index, current->pid, curr_device->setted_hash_size
			);
			return -EINVAL;
	}

	// Make sure that the user requested at least as many 
	// bytes as the output, otherwise we might copy more
	// than the user asked, which is dangerous
	if (user_len < hash_size_bytes) {
		return -EINVAL;
	}
	
	// 1. Set control register to appropriate value

	// Bytes left to send
	control_value |= curr_device->data_to_send_length;

	// This is the last packet
	control_value |= 1 << 2;

	// Hash size
	control_value |= curr_device->setted_hash_size << 4;
	writel(control_value, curr_device->control);
	kc_info("[ketchup_read] writing %08x to control\n", control_value);

	// 2. Send last packed of input data
	packed_input = pack_to_u32_big_endian(curr_device->data_to_send);
	writel(packed_input, curr_device->input);
	kc_info("[ketchup_read] writing %08x to input\n", packed_input);
	
	// 3. Wait for polling
	for (int i = 0; i < 100; i++) {
		kc_info("[ketchup_read] polling iter %d\n", i);
		if ((readl(curr_device->status) & 1) != 0) {
			break;
		}
		if (i == 99) {
			kc_err("[ketchup_read] something went wrong, stop after 100 iters");
		}
	}

	// 4. Get output from peripheral
	for (int i = 0; i < hash_size_bytes/4; i++) {
		output = readl(curr_device->output_base + i * 4);
		output_buffer[i * 4 + 0] = (output >> 24) & 0xFF;
		output_buffer[i * 4 + 1] = (output >> 16) & 0xFF;
		output_buffer[i * 4 + 2] = (output >> 8)  & 0xFF;
		output_buffer[i * 4 + 3] = output         & 0xFF;
	}

	// 5. Send output to user
	data_to_copy = min(hash_size_bytes, user_len);
	error = copy_to_user(user_buffer, output_buffer, data_to_copy);
	// TODO: Handle Error
	if (error != 0) {
		kc_err("[ketchup_read] copy_to_user failed. retval = %d\n", error);
	}

	// 6. Reset the peripheral, for good measure
	writel(1, curr_device->command);
	curr_device->data_to_send_length = 0;

	// 7. Reset previous hash size into control
	control_value = curr_device->setted_hash_size << 4;
	writel(control_value, curr_device->control);
	kc_info("[ketchup_read] writing %08x to control\n", control_value);

	return data_to_copy;
}

/**
 * This function is called when a file descriptor opened of our character device file 
 * (i.e /dev/ketchup_driver) is closed.
 * What we do is we reset the peripheral and than we mark is as available inside the 
 * array
*/
static int ketchup_release(struct inode *inod, struct file *fil)
{
	int assigned_periph_index = (int)(uintptr_t)fil->private_data;

	peripheral_release(assigned_periph_index);

	return 0;
}

// ========================== sysfs =============================

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
	uint32_t hash_size_mask;
	uint32_t control_register_value = 0;
	uint32_t extracted_bits = 0;
	struct ketchup_device * curr_device;
	struct ketchup_devices_container *container;
	char buffer[200];
	int len = 0;

	container = &ketchup_drvr_data.devices;

	mutex_lock(&container->array_write_lock);

	for (int index = 0; index < container->registered_devices_len; index++){
		curr_device = container->registered_devices[index];

		control_register_value = readl(curr_device->control);
		hash_size_mask = 0x30; // 0b_0011_0000
		extracted_bits = (control_register_value & hash_size_mask) >> 4;
		// 00 for 512, 01 for 384, 10 for 256 and 11 for 224
		switch (extracted_bits)
		{
			case 0:
				/* code */
				len += snprintf(buffer + len, sizeof(buffer) - len, "Hash_size[%d] = 512\n", index);
				break;
			case 1:
				len += snprintf(buffer + len, sizeof(buffer) - len, "Hash_size[%d] = 384\n", index);
				break;
			case 2:
				len += snprintf(buffer + len, sizeof(buffer) - len, "Hash_size[%d] = 256\n", index);
				break;
			case 3:
				len += snprintf(buffer + len, sizeof(buffer) - len, "Hash_size[%d] = 224\n", index);
				break; 
			default:
				len += snprintf(buffer + len, sizeof(buffer) - len, "Error!\n");
				break;
		}
	}

	mutex_unlock(&container->array_write_lock);

	strcpy(buf, buffer);
	return len;
}

/**
 * Writable, it's always zero, OTHERS_WRITABLE? BAD IDEA - kernel
 * I'm also resetting the hash size to 512 don't ask me why, probably a bad idea since i'm 
 * changing the meaning of the control register but it's an easy and quick way of resetting
 * a peripheral when testing
*/
/*
static DEVICE_ATTR(command, 0220, NULL, write_command);
static ssize_t write_command(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int peripheral;
	uint32_t reset;
	struct ketchup_device *curr_device;
	struct ketchup_devices_container *container;

	sscanf(buf, "%d,%d", &peripheral, &reset);

	kc_info("[write_command] called!\n");
	kc_info("[write_command] peripheral_argument: %d\n", peripheral);
	kc_info("[write_command] reset_argument: %d\n", reset);

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
	writel((uint32_t)0, ketchup_drvr_data.all_registered_peripherals[peripheral].control);
	#ifdef KECCAK_DEBUG
	pr_info("Peripheral number %d reset completed\n", peripheral);
	#endif
	return strlen(buf);
}
*/

/**
 * This attributes reveals the current usage of all the peripherals
 * When called you will be able to see all the peripherals and the 
 * pid of the process currently using it
*/
static DEVICE_ATTR(current_usage, 0444, read_current_usage, NULL);
static ssize_t read_current_usage(struct device *dev, struct device_attribute *attr, char *buf)
{
	// Abbiamo un array, andiamo dentro e ci prendiamo i dati
	char buffer[200];
	int len = 0;
	struct ketchup_devices_container *container = &ketchup_drvr_data.devices;
	struct ketchup_device *curr_device;

	mutex_lock(&container->array_write_lock);

	for (int i = 0; i < container->registered_devices_len; i++){
		curr_device = container->registered_devices[i];

		if(curr_device->current_process == 0){
			// If we are here it means that the peripheral is not in usage
			len += snprintf(buffer + len, sizeof(buffer) - len, "%d:\n", i);
		} else {
			// ne aggiungi un'altra
			len += snprintf(buffer + len, sizeof(buffer) - len, "%d:%d\n", i, curr_device->current_process);
		}
	}

	mutex_unlock(&container->array_write_lock);

	// Now we need to copy data to the user space
	strcpy(buf, buffer);
	return len;
}

// ====================== Device Probing ==============================

/**
 * Probe function that's called after the init function when
 * the driver is loaded. 
 * It's run once for every device compatible with out driver
*/
static int ketchup_driver_probe(struct platform_device *pdev)
{
	struct resource *r_mem; /* IO mem resources */
	struct device *dev = &pdev->dev;
	struct ketchup_devices_container *container;
	struct ketchup_device *lp = NULL;
	int rc = 0;

	// Get iospace for the device from the device treee
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r_mem) {
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}

	// Allocate space to accomodate one peripheral description 
	lp = (struct ketchup_device *) kmalloc(sizeof(struct ketchup_device), GFP_KERNEL);
	if (!lp) {
		dev_err(dev, "Cound not allocate ketchup-driver device\n");
		return -ENOMEM;
	}

	dev_set_drvdata(dev, lp);
	lp->mem_start = r_mem->start;
	lp->mem_end = r_mem->end;

	// Map physical memory region to kernel space

	// First check if region is available
	if (!request_mem_region(lp->mem_start,
				lp->mem_end - lp->mem_start + 1,
				DRIVER_NAME)) {
		dev_err(dev, "Couldn't lock memory region at %p\n",
			(void *)lp->mem_start);
		rc = -EBUSY;
		goto error1;
	}

	// Then actually remap it
	lp->base_addr = ioremap(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	if (!lp->base_addr) {
		dev_err(dev, "ketchup-driver: Could not allocate iomem\n");
		rc = -EIO;
		goto error2;
	}

	// Save all register positions
	lp->control = lp->base_addr;
	lp->status = lp->base_addr + 4;
	lp->input = lp->base_addr + 8;
	lp->command = lp->base_addr + 12;
	lp->output_base = lp->base_addr + 16;

	// Initialize device state
	lp->peripheral_available = AVAILABLE;
	lp->current_process = 0;
	lp->data_to_send_length = 0;
	

	// Save device into container
	container = &ketchup_drvr_data.devices;
	mutex_lock(&container->array_write_lock);
	if (container->registered_devices_len >= MAX_DEVICES) {
		kc_err("[ketchup_driver_probe] too many devices, %d is the current limit\n", MAX_DEVICES);
		mutex_unlock(&container->array_write_lock);

		// FIXME: Probably not the correct return value here
		return -EINVAL;
	}

	container->registered_devices[container->registered_devices_len] = lp;
	container->registered_devices_len++;

	kc_info("[ketchup_driver_probe] registered device number %d\n", container->registered_devices_len);
	mutex_unlock(&container->array_write_lock);

	up(&container->dev_free_sema);

	/* 
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
	*/

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
	struct ketchup_device *curr_device;
	struct ketchup_devices_container *container = &ketchup_drvr_data.devices;

	// This is not exactly correct, but we're going to remove all peripherals
	// even when just one is removed. We somehow need a better way to do this
	kc_info("[ketchup_driver_remove] removing all peripherals\n");

	mutex_lock(&container->array_write_lock);

	if (container->registered_devices_len <= 0) {
		mutex_unlock(&container->array_write_lock);
		return 0;
	}

	for (int i = 0; i < container->registered_devices_len; i++) {
		curr_device = container->registered_devices[i];
		iounmap(curr_device->base_addr);
		release_mem_region(curr_device->mem_start, curr_device->mem_end - curr_device->mem_start + 1);
		kfree(curr_device);
	}
	container->registered_devices_len = 0;
	mutex_unlock(&container->array_write_lock);

	dev_set_drvdata(dev, NULL);
	return 0;
}

static char *ketchup_devnode(struct device *dev, umode_t *mode)
{
    if (!mode)
        return NULL;

	// rw- rw- rw-
    *mode = 0666;

    return NULL;
}

// ===================== Module Functions ==========================

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
	ketchup_drvr_data.driver_class->devnode = ketchup_devnode;

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
	/*
	if (device_create_file(ketchup_drvr_data.test_device, &dev_attr_command) < 0)
	{
		pr_err("ketchup_driver_init: command attribute creation failed\n");
	}
	*/

	if (device_create_file(ketchup_drvr_data.test_device, &dev_attr_current_usage) < 0)
	{
		pr_err("ketchup_driver_init: current_usage attribute creation failed\n");
	}

	// Initialize devices container
	mutex_init(&ketchup_drvr_data.devices.array_write_lock);
	sema_init(&ketchup_drvr_data.devices.dev_free_sema, 0);


	return platform_driver_register(&ketchup_driver_driver);
}

/**
 * Exit function, called when we run "rmmod ketchup-driver"
*/
static void __exit ketchup_driver_exit(void)
{
	cdev_del(&ketchup_drvr_data.c_dev);
	device_remove_file(ketchup_drvr_data.test_device, &dev_attr_control);
	// device_remove_file(ketchup_drvr_data.test_device, &dev_attr_command);
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

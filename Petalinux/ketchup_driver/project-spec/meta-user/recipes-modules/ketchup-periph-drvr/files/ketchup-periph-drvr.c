// Global Module Utils
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>

// Probing and Memory Mapping
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/io.h>

// Character Device and Platform Device
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ioctl.h>

// Concurrency Primitives
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/signal.h>

// Utilities
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>

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

// How big the buffer for copying data to kernel space is
#define KC_BUF_SIZE 1024

// How many devices we can support at maximum
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
	HashSize hash_size;
};

/**
 * This is a collection of all registered peripherals
*/ 
struct ketchup_devices_container {
	// Used to protect the registered_devices array
	struct mutex array_write_lock;

	// Used to make sure that only registered_devices_len
	// processes at a time can access a peripheral
	struct semaphore dev_free_sema;

	struct ketchup_device *registered_devices[MAX_DEVICES];
	size_t registered_devices_len;
};

// This struct hold the global data
// needed for the driver
static struct char_dev {
    struct class *driver_class;

	// Contains both the major number and 
	// the minor number for our device
    dev_t device_number;

	// Representing the character device
    struct cdev c_dev;

	// This is the device file registered with
	// sysfs, and represents the file that will
	// show up inside /dev
    struct device *registered_device;

	// This is a container for all registered peripherals,
	// with some concurrency primitives to keep processes 
	// from accessing the same peripheral at the same time
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
	device->hash_size = HASH_512;

	// Clears internal state and output
	writel(1, device->command);
}

/**
 * This function is called only by ketchup_open.
 * Checks if any peripherals are available to use, and assigns one
 * to the process requesting it. If there are none, it either makes
 * the process sleep until one becomes available, or if nonblocking
 * behaviour is requested returns -EAGAIN. 
 * This returns the index of the assigned peripheral on success.
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
			curr_device->hash_size = HASH_512;
			
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

/** 
 * Releases the peripheral for future use. It also clears the peripheral
 * to make sure that no leftover data can be accessed from any future user.
*/
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

/**
 * Small utility that retrieves a pointer to the peripheral owned by a fd
*/
static inline struct ketchup_device *kc_get_device(struct file *filep) {
	int assigned_periph_index = (int)(uintptr_t)filep->private_data;
	return ketchup_drvr_data.devices.registered_devices[assigned_periph_index];
}

/**
 * This is the function that is called whenever a new open syscall is made to our
 * driver device file (/dev/kechtup_driver).
*/
static int ketchup_open(struct inode *inod, struct file *fil)
{
	/**
	 * When a new file descriptor is opened we need to search for an available 
	 * peripheral that can serve the request.
	 * If all the peripherals are already assigned, we return -EAGAIN
	*/
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
 * commands from one another in case of an ioctl to the wrong device.
 * We chose 0xFC only because it's not used by anything else in the kernel by default.
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
			curr_device->hash_size = command;
			command = command << 4;
			writel(command, curr_device->control);
			break;
		case RD_PERIPH_HASH_SIZE:
			// We want to return the value of the hash size
			command = curr_device->hash_size;
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

/**
 * Finds the minimum between two numbers. For some reason the kernel implementation 
 * was giving me some warnings, so I opted to implement it myself
*/
#define kc_min(x, y) ((x) < (y) ? (x) : (y))

/**
 * Packs a byte array into a big endian uint32_t number.
 * NOTE: This expects the array to be at least 4 long.
*/
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
	 * 2. we send all the data we got from the user to the peripheral, alongside any residual
	 * unaligned data from the previous write call if present.
	*/
	uint8_t buffer[KC_BUF_SIZE];
	size_t buffer_length, buffer_position;
	int error;
	int peripheral_index = (int)(uintptr_t)filep->private_data;

	// We need to retrieve from the file descriptor the peripheral index assigned
	struct ketchup_device *curr_device = kc_get_device(filep);

	kc_info(
		"[ketchup_write] beginning write for peripheral %d task %d. hash_size = %d\n",
		peripheral_index, current->pid, curr_device->hash_size
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
 * 1. Set this write as the last one;
 * 2. Send the last packet of input data to the peripheral;
 * 3. Start polling the peripheral on the status register;
 * 4. Once the output is ready, we save the output into a buffer;
 * 5. We copy the output buffer to user space;
 * 6. Reset the peripheral;
 * 7. Set the same hash size as before in control.
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
	switch (curr_device->hash_size)
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
				assigned_periph_index, current->pid, curr_device->hash_size
			);
			return -EINVAL;
	}

	// Make sure that the user requested at least as many 
	// bytes as the output, otherwise we might copy more
	// than the user asked, which is potentially dangerous
	if (user_len < hash_size_bytes) {
		return -EINVAL;
	}
	
	// 1. Set control register to appropriate value

	// Bytes left to send
	control_value |= curr_device->data_to_send_length;

	// This is the last packet
	control_value |= 1 << 2;

	// Hash size
	control_value |= curr_device->hash_size << 4;
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

	if (error < 0) {
		kc_err("[ketchup_read] copy_to_user failed. retval = %d\n", error);
		return error;
	}

	// 6. Reset the peripheral, for good measure
	writel(1, curr_device->command);
	curr_device->data_to_send_length = 0;

	// 7. Reset previous hash size into control
	control_value = curr_device->hash_size << 4;
	writel(control_value, curr_device->control);
	kc_info("[ketchup_read] writing %08x to control\n", control_value);

	return data_to_copy;
}

/**
 * This function is called when a file descriptor opened of our character device file 
 * (i.e /dev/ketchup_driver) is closed.
 * What we do is we reset the peripheral and than we release the acquired peripheral
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
 * The DEVICE_ATTR macro arguments are attribute_name, r/w, read_callback, write_callback
 * Remember that each attribute will expand in dev_attr_(attribute-name) so add the
 * correct code in the init and exit functions.
*/

/**
 * This declares the attribute as read only. This will expand 
 * into dev_attr_hash_size, and will call the function hash_size_show.
 * It also declares the permissions as r-- r-- r-- (0444)
*/
static DEVICE_ATTR_RO(hash_size);
static ssize_t hash_size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ketchup_devices_container *container;
	struct ketchup_device * curr_device;
	// ~ 20 char for device
	char buffer[MAX_DEVICES * 24];
	int len = 0;

	container = &ketchup_drvr_data.devices;

	mutex_lock(&container->array_write_lock);

	for (int index = 0; index < container->registered_devices_len; index++){
		curr_device = container->registered_devices[index];
		switch (curr_device->hash_size)
		{
			case HASH_512:
				/* code */
				len += snprintf(buffer + len, sizeof(buffer) - len, "HashSize[%d] = 512\n", index);
				break;
			case HASH_384:
				len += snprintf(buffer + len, sizeof(buffer) - len, "HashSize[%d] = 384\n", index);
				break;
			case HASH_256:
				len += snprintf(buffer + len, sizeof(buffer) - len, "HashSize[%d] = 256\n", index);
				break;
			case HASH_224:
				len += snprintf(buffer + len, sizeof(buffer) - len, "HashSize[%d] = 224\n", index);
				break; 
			default:
				kc_err(
					"[sysfs_hash_size] invalid hash size for peripheral %d: %d\n", 
					index, 
					curr_device->hash_size
				);
				len += snprintf(buffer + len, sizeof(buffer) - len, "HashSize[%d] = invalid\n", index);
				break;
		}
	}

	mutex_unlock(&container->array_write_lock);

	strcpy(buf, buffer);
	return len;
}


static DEVICE_ATTR_RO(current_usage);
/**
 * This attributes reveals the current usage of all the peripherals
 * When called you will be able to see all the peripherals and the 
 * pid of the process currently using it
*/
static ssize_t current_usage_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ketchup_device *curr_device;
	struct ketchup_devices_container *container = &ketchup_drvr_data.devices;
	// ~ 15 char per device, conservative estimate
	char buffer[MAX_DEVICES * 15];
	int len = 0;

	mutex_lock(&container->array_write_lock);

	for (int i = 0; i < container->registered_devices_len; i++){
		curr_device = container->registered_devices[i];

		// If the peripheral is available, it means 
		// that no process is currently holding it
		if(curr_device->peripheral_available == AVAILABLE){
			len += snprintf(buffer + len, sizeof(buffer) - len, "%d:\n", i);
		} else {
			len += snprintf(buffer + len, sizeof(buffer) - len, "%d:%d\n", i, curr_device->current_process);
		}
	}

	mutex_unlock(&container->array_write_lock);

	// Now we need to copy data to the user space.
	// No need for copy_to_user here. 
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
 * init function, it's the first function that is called when the driver is loaded
*/
static int __init ketchup_driver_init(void)
{
	kc_info("[ketchup_driver_init] starting up...\n");
	/**
	 * Allocates both a major and minor number for our device dynamically
	 * The parameters are:
	 * - output parameter for the first assigned number
	 * - first of the requested range of minor numbers
	 * - the number of minor numbers required
	 * - the name of the associated device or driver
	*/
	if (alloc_chrdev_region(&ketchup_drvr_data.device_number, 0, 1, DRIVER_NAME) < 0)
	{
		kc_err("[ketchup_driver_init] could not allocate device number\n");
		return -1;
	}

	kc_info(
		"[ketchup_driver_init] device number = %d, major = %d, minor = %d\n", 
		ketchup_drvr_data.device_number,
		MAJOR(ketchup_drvr_data.device_number),
		MINOR(ketchup_drvr_data.device_number)
	);

	/**
	 * Create the device class
	*/
	ketchup_drvr_data.driver_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(ketchup_drvr_data.driver_class))
	{
		kc_err("[ketchup_driver_init] could not create class\n");
		unregister_chrdev_region(ketchup_drvr_data.device_number, 1);
		return -1;
	}

	/**
	 * Setting the correct file permissions
	*/
	ketchup_drvr_data.driver_class->devnode = ketchup_devnode;

	/**
	 * Register the device with sysfs, this will create the file in /dev
	*/
	ketchup_drvr_data.registered_device = device_create(
		ketchup_drvr_data.driver_class,   
		NULL,                             // Parent  
		ketchup_drvr_data.device_number,  
		NULL,                             // drvdata
		DEVICE_NAME	
	);
	if (IS_ERR(ketchup_drvr_data.registered_device))
	{
		kc_err("[ketchup_driver_init] device initialization failed\n");
		class_destroy(ketchup_drvr_data.driver_class);
		unregister_chrdev_region(ketchup_drvr_data.device_number, 1);
		return -1;
	}

	// Initialize the character device
	cdev_init(&ketchup_drvr_data.c_dev, &fops);

	if (cdev_add(&ketchup_drvr_data.c_dev, ketchup_drvr_data.device_number, 1) == -1)
	{
        kc_err("[ketchup_driver_init] cdev initialization failed\n");
		device_destroy(ketchup_drvr_data.driver_class, ketchup_drvr_data.device_number);
		class_destroy(ketchup_drvr_data.driver_class);
		unregister_chrdev_region(ketchup_drvr_data.device_number, 1);
		return -1;
	}

	// control, command, current_usage
	if (device_create_file(ketchup_drvr_data.registered_device, &dev_attr_hash_size) < 0)
	{
        kc_err("[ketchup_driver_init] control sysfs initialization failed\n");
	}

	if (device_create_file(ketchup_drvr_data.registered_device, &dev_attr_current_usage) < 0)
	{
        kc_err("[ketchup_driver_initb current_usage initialization failed\n");
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
	device_remove_file(ketchup_drvr_data.registered_device, &dev_attr_hash_size);
	device_remove_file(ketchup_drvr_data.registered_device, &dev_attr_current_usage);
	device_destroy(ketchup_drvr_data.driver_class, ketchup_drvr_data.device_number);
	class_destroy(ketchup_drvr_data.driver_class);
	platform_driver_unregister(&ketchup_driver_driver);

	kc_info("[ketchup_driver_exit] Module unloaded\n");
}

module_init(ketchup_driver_init);
module_exit(ketchup_driver_exit);

#include "asm-generic/errno-base.h"
#include "linux/device/class.h"
#include "linux/err.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

// Module Configuration
#define DEVICE_NAME "example_module"
#define CLASS_NAME "example"
#define MODULE_NAME "example"

#define LOG_INFO KERN_INFO MODULE_NAME ": "
#define LOG_ALERT KERN_ALERT MODULE_NAME ": "

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daem");
MODULE_DESCRIPTION("An example linux driver");
MODULE_VERSION("0.1");

// Globals
static int major_number;
static struct class *char_class = NULL; 
static struct device *char_device = NULL;

// Device functions
static int example_open_close(struct inode *, struct file *);
static ssize_t example_read(struct file *, char *, size_t, loff_t *);
static ssize_t example_write(struct file *, const char *, size_t, loff_t *);
static long int example_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = example_open_close,
    .release = example_open_close,
    .read = example_read,
    .write = example_write,

    .compat_ioctl = example_ioctl,
    .unlocked_ioctl = example_ioctl
};

// used to issue commands to the driver (e.g. select a specific register based on the value of command)
static long int example_ioctl(struct file *file_ptr, unsigned int command, unsigned long arg) {

    printk(LOG_INFO "example: Executing ioctl\n");
    return -1;
}
static int example_open_close(struct inode *inodep, struct file *file_ptr) {
    printk(LOG_INFO "example: Executing open/release\n");
    return 0;
}

static ssize_t example_read(struct file *file_ptr, char *buffer, size_t len, loff_t *offset) {
    printk(LOG_INFO "Executing read\n");

    char in_buf[] = "Hello World!\n";
    size_t msg_len = strlen(in_buf);

    size_t bytes_to_copy = len <= msg_len ? len : msg_len;
    printk(LOG_INFO "len: %lu, msg_len: %lu, bytes_to_copy: %lu\n", len, msg_len, bytes_to_copy);

    int errors = copy_to_user(buffer, in_buf, bytes_to_copy);

    return errors == 0 ? bytes_to_copy : -EFAULT;
}

static ssize_t example_write(struct file *, const char *, size_t, loff_t *) {
    printk(LOG_INFO "Executing write\n");

    return 1;
}

static int __init example_init(void) {
    // Register the major number. 0 for auto-assignment by kernel
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(LOG_ALERT "Failed to register a major number. Error = %d\n", major_number);
        return major_number;
    }


    // Register the device class
    char_class = class_create(CLASS_NAME);
    if (IS_ERR(char_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(LOG_ALERT "Failed to register the device class\n");
        return PTR_ERR(char_class);
    }

    // Create the character device (the actual file in /dev)
    char_device = device_create(char_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(char_device)) {
        class_destroy(char_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(LOG_ALERT "Failed to create the device\n");
        return PTR_ERR(char_device);
    }

    // Specific functions to initialize the hardware should be done here

    printk(LOG_INFO "Module has been loaded. major = %d\n", major_number);
    return 0;
}


static void __exit example_exit(void) {
    device_destroy(char_class, MKDEV(major_number, 0));
    class_unregister(char_class);
    class_destroy(char_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(LOG_INFO "Module unloaded\n");
}

module_init(example_init)
module_exit(example_exit)
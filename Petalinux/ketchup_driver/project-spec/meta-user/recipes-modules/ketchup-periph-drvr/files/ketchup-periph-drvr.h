#ifndef KETCHUP_DRIVER
#define KETCHUP_DRIVER

/******************* DEFINES *******************/
/**
 * DRIVER_NAME -> name of the driver
 * CLASS_NAME -> name of the class in /sys/class/
*/
#define DRIVER_NAME "ketchup-driver"
#define DEVICE_NAME "ketchup_driver"
#define CLASS_NAME "keccak_accelerators"

// Enable this for debugging
#define KECCAK_DEBUG


/******************* FUNCTIONS *******************/

// Module Functions
static int __init ketchup_driver_init(void);
static void __exit ketchup_driver_exit(void);

// Character device
static int ketchup_open(struct inode *, struct file *);
static ssize_t ketchup_read(struct file *, char *, size_t, loff_t *);
static ssize_t ketchup_write(struct file *, const char *, size_t, loff_t *);
static int ketchup_release(struct inode *, struct file *);
static long ketchup_ioctl(struct file *, unsigned int, unsigned long);

// Platform device
static int ketchup_driver_probe(struct platform_device *);
static int ketchup_driver_remove(struct platform_device *);

// sysfs
static ssize_t read_control(struct device *, struct device_attribute *, char *);
// static ssize_t write_command(struct device *, struct device_attribute *, const char *, size_t);
static ssize_t read_current_usage(struct device *, struct device_attribute *, char *);


typedef enum {
    NOT_AVAILABLE,
    AVAILABLE
} Availability;

typedef enum {
    HASH_512 = 0,
    HASH_384 = 1,
    HASH_256 = 2,
    HASH_224 = 3
} HashSize;
#endif
#ifndef KETCHUP_DRIVER
#define KETCHUP_DRIVER

/******************* DEFINES *******************/
/**
 * DRIVER_NAME -> name of the driver
 * CLASS_NAME -> name of the class in /sys/class/
*/
#define DRIVER_NAME "ketchup-driver"
#define CLASS_NAME "KEKKAK_ACCELERATORS"
#define DEBUG
#define NUM_INSTANCES 5
#define BUF_SIZE 1024
/******************* FUNCTIONS *******************/
static int dev_open(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static int dev_release(struct inode *, struct file *);
static ssize_t read_control(struct device *, struct device_attribute *, char *);
static ssize_t read_status(struct device *, struct device_attribute *, char *);
static ssize_t write_command(struct device *, struct device_attribute *, const char *, size_t);
static int ketchup_driver_probe(struct platform_device *);
static int ketchup_driver_remove(struct platform_device *);
static int __init ketchup_driver_init(void);
static void __exit ketchup_driver_exit(void);
int peripheral_array_access(struct file *);
static ssize_t read_current_usage(struct device *, struct device_attribute *, char *);
void write_into_input_reg(char []);
int peripheral_release(struct file *);
typedef enum {
    NOT_AVAILABLE,
    AVAILABLE
} Availability;
#endif
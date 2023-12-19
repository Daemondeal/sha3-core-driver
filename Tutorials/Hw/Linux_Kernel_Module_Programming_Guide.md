# The Linux Kernel Module Programming Guide

**Kernel module**: code segment capable of dynamically loading/unloading from the kernel without requiring a reboot.

The alternative to kernel modules would be a monolithic kernel where every module needs to be built inside the kernel image.

## Useful commands

- `modprobe`: it's better than _insmod_ because it will load the module along with eventual dependencies
- `insmod`: it will insert the module
- `depmod`: i think it shows the dependencies of a module, never tried it

You can use `lsmod` to get a **list of loaded kernel modules**.
You can use `cat /proc/modules` to peek inside the modules file where i guess there is the same thing of _lsmod_.

## Notices

1. A module built for a specific kernel version will not work if you change the version, except if `CONFIG_MODVERSIONS` is enabled in the kernel (refer to versioning)
2. You may need to disable SecureBoot

## HelloWorld module

```c
#include <linux/module.h> /* Necessary for all modules */
#include <linux/printk.h> /* Needed for pr_info() */

int init_module(void){
	pr_info("Ciao dal mio modulo helloworld\n");
	/* Se ritorni non zero vuol dire che il caricamento del modulo e' fallito */
	return 0;
}

void cleanup_module(void){
	pr_info("Dealloco il modulo helloworld \n");
}
MODULE_LICENSE("GPL");
```

What all kernel modules need to have is a `init_module()` and a `cleanup_module()` function. You can also give it a different name but we'll see that later.
The idea of the init functions is to do something with the kernel (handler or replacing a default function) and the cleaup has to undo what init did.

> Inside `include/linux/printk.h` you'll find all the different priority levels for `pr_info`. pr_info == printk(KERN_INFO), pr_debug == printk(KERN_DEBUG)

We can do better than this, what if we don't want to use the specific name of init_module and cleanup_module?

```c
#include <linux/init.h> /* Necessary for the renaming of the functions*/
#include <linux/module.h> /* Necessary for all modules */
#include <linux/printk.h> /* Needed for pr_info() */

static int data __initdata = 3;

static int __init custom_name_init(void){
	pr_info("Ciao dal mio modulo helloworld\n");
	/* Se ritorni non zero vuol dire che il caricamento del modulo e' fallito */
	return 0;
}

static void __exit custon_name_cleaup(void){
	pr_info("Dealloco il modulo helloworld \n");
}

module_init(custom_name_init); // binding
module_exit(custon_name_cleanup); // binding

MODULE_LICENSE("GPL");

```

Basically we did the same thing of before but by using the **init and **exit **compiler directives**. What does this mean? It means that by writing **init or **exit we are telling the compiler to put our function in a specific part of the final compiled module. Why would we want to do this? Because when the kernel load it executes all the things inside the init portion and than after it's done it can free all of this memory since it won't be needed again.

> All of this only has sense if the module is **static** > **Don't use "module" in the module name**

## Licensing

You can add things like

```c
MOUDLE_LICENSE("GPL");
MODULE_AUTHOR("LORENZO");
MODULE_DESCRIPTION("Hello world driver");
```

All of the possible values/information are found inside the `include/linux/module.h`.
You can put all of this "module descriptions" also before all the code.

## Command line arguments

It's possible for a module to receive command line arguments.
How? Just

1. Declare as **global** the variables that will take the value passed in
2. Use `module_param()` macro (defined in `include/linux/moduleparam.h`)
3. When inserting the module, type something like `insmod module.ko variable=3`

If you want to do something like this, refer to the book online.

## Modules spanning multiple files

We can have something like

```c
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL")

int init_module(void){
    pr_info("Hello\n");
    return 0;
}
```

and

```c
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL")

void cleanup_module(void){
    pr_info("Adiosss\n");
}
```

> Oh wow you splitted the module in multiple files xd. But you'll need to modify the `modulename.bb` by adding all the files that compose the module in the `SRC_URI` and than in the Makefile you'll need to add the new object files that have to be generated, something like

```make
obj-m := helloworld_init.o
obj-m := helloworld_cleanup.o
obj-m := final.o
final-objs := helloworld_init.o helloworld_cleanup.o
```

## Life cycle of a kernel module

It all starts with the `init_module` or with the function specified in the `module_init(function_name)`. In this function you tell the kernel what functionality the module provides and sets up the kernel to run the module's functions.

The module at this point does nothing until the kernel wants to do something with it.

When the module has to go, the `cleanup_module` functions is called and the module is unloaded (basically it undoes whatever the entry function did and it unregisters the functionality that the entry function registered).

## Functions available inside modules

Modules are .o files who's symbols get resolved upon running `insmod/modprobe`, the only external functions i can use are those defined inside the kernel.

Basically the functions available inside the kernel are like syscalls. If you want to see which syscalls a normal c program uses just compile it and than run it with `strace ./executable_name` and you will get a list of all the syscall that are being used with the arguments and what it returns.

## Variable names collision

Since all the variables in my module will be linked against the entire kernel, declare them as **static** to dodge problems.

> `/proc/kallsyms` holds all the symbols that the kernel knows about

## Device drivers

They are a particular class of kernel modules. If you `ls` inside of the `/dev/` folder, you'll see different numbers.
The first one is the **major number**, it tells us which driver is used to access the hardware. The second number is the **minor number** and is used by the driver to distinguish between various hardware it controls.

The devices are divided into two classes:

- **block devices**: they have a buffer for requests so they can choose the best order in which to respond to the requests (example: storage device, it's faster to access contiguos sectors than more far apart ones) and they can only accept input and return output in blocks (when you `ls` in /dev/ they are the ones that start with **b** )
- **character devices**: they can use as many or as few bytes as they want (when you `ls` in /dev/ they are the ones that start with **c**) == to access them you do read/write

## Character devices drivers

A character device is managed through the **file_operations** structure defined in `include/linux.h`. This structure holds pointers to the various functions defined inside the driver that perform various operations on the device.

An example

```c
struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
}
```

Any member of the struct not specifically assigned will be initialized to NULL.

> `fops` is the common name for this structure

> The `read` and `write` operations are guaranteed to be thread safe by the OS, you don't need to do explicit locking/unlocking

> Each device is represented inside the kernel as a **file** structure you can find in `include/linux/fs.h`, an instance of this file structure is called **flip**

Char devices are accessed through device files located in `/dev/`. You can theoretically also put it elsewhere even tho i don't know how. Anyway, in order to add a driver you first need to **registering it** with the kernel by

```c
int register_chrdev(unsigned int major, const char *name, struct file_operations *fops)
```

> A negative return value means the registration failed

with `major` as the major number, `name` as the name of the driver (the name that will appear in `/proc/devices`) and `fops` is the pointer to the `file_operations` struct.

Which major number to choose? You don't need to choose anything, just pass 0 as the major number and the kernel will assign one automatically.

Now that we have a major number, we still have the problem of creating the device in `/dev/`. The easiest way is to use `device_create()` and `device_destroy()` after a successful registration and during the call to cleanup respectively.

Example:

```c
/*
 * chardev.c: Creates a read-only char device that says how many times
 * you have read from the dev file
 */

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h> /* for sprintf() */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/version.h>
#include <asm/errno.h>

/*  Prototypes - this would normally go in a .h file */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev" /* Dev name as it appears in /proc/devices */
#define BUF_LEN 80 /* Max length of the message from the device */

/* Global variables are declared as static, so are global within the file.*/
static int major; /* major number assigned to our device driver */

enum {
    CDEV_NOT_USED = 0,
    CDEV_EXCLUSIVE_OPEN = 1,
};

/* Is device open? Used to prevent multiple access to device */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char msg[BUF_LEN + 1]; /* The msg the device will give when asked */

static struct class *cls;
static struct file_operations chardev_fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

static int __init chardev_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &chardev_fops);
    if (major < 0) {
        pr_alert("Registering char device failed with %d\n", major);
        return major;
    }
    pr_info("I was assigned major number %d.\n", major);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cls = class_create(DEVICE_NAME);
#else
    cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    pr_info("Device created on /dev/%s\n", DEVICE_NAME);
    return SUCCESS;

}

static void __exit chardev_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    /* Unregister the device */
    unregister_chrdev(major, DEVICE_NAME);
}

/* Methods */
/* Called when a process tries to open the device file, like
 * "sudo cat /dev/chardev"
 */
static int device_open(struct inode *inode, struct file *file)
{
    static int counter = 0;
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;
    sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    try_module_get(THIS_MODULE);
    return SUCCESS;
}

/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file)
{
    /* We're now ready for our next caller */
    atomic_set(&already_open, CDEV_NOT_USED);
    /* Decrement the usage count, or else once you opened the file, you will
     * never get rid of the module.
     */
    module_put(THIS_MODULE);
    return SUCCESS;
}

/* Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */
                           char __user *buffer, /* buffer to fill with data */
                           size_t length, /* length of the buffer     */
                           loff_t *offset)
{
    /* Number of bytes actually written to the buffer */
    int bytes_read = 0;
    const char *msg_ptr = msg;
    if (!*(msg_ptr + *offset)) { /* we are at the end of message */
        *offset = 0; /* reset the offset */
        return 0; /* signify end of file */
    }
    msg_ptr += *offset;
    /* Actually put the data into the buffer */
    while (length && *msg_ptr) {
        /* The buffer is in the user data segment, not the kernel
         * segment so "*" assignment won't work.  We have to use
         * put_user which copies data from the kernel data segment to
         * the user data segment.
         */
        put_user(*(msg_ptr++), buffer++);
        length--;
        bytes_read++;
    }
    *offset += bytes_read;
    /* Most read functions return the number of bytes put into the buffer. */
    return bytes_read;
}

/* Called when a process writes to dev file: echo "hi" > /dev/hello */
static ssize_t device_write(struct file *filp, const char __user *buff,
                            size_t len, loff_t *off)
{
    pr_alert("Sorry, this operation is not supported.\n");
    return -EINVAL;
}

module_init(chardev_init);
module_exit(chardev_exit);
MODULE_LICENSE("GPL");
```

## `/proc` file system

There is another way to send infos from the kernel/kernel modules to processes: using the `/proc` file system. I don't think we'll use this way so go to the book to find it.

## Interacting with the module (`sysfs`)

It's possible from userspace to read/set variables inside modules, the way we do that is by exporting attributes in the form of regular files in the filesystem, than `sysfs` will forward file I/O operations to functions defined for each attribute.

What are these attributes?
Simply

```c
struct attribute {
    char *name;
    struct module *owner;
    umode_t mode;
};

int sysfs_create_file(struct kobject * kobj, const struct attribute * attr);
void sysfs_remove_file(struct kobject * kobj, const struct attribute * attr);
```

And so an example of usage would be

```c
/*
 * hello-sysfs.c sysfs example
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>

static struct kobject *mymodule;
/* the variable you want to be able to change */
static int myvariable = 0;

static ssize_t myvariable_show(struct kobject *kobj,
                               struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", myvariable);
}

static ssize_t myvariable_store(struct kobject *kobj,
                                struct kobj_attribute *attr, char *buf,
                                size_t count)
{
    sscanf(buf, "%du", &myvariable);
    return count;
}

static struct kobj_attribute myvariable_attribute = __ATTR(myvariable, 0660, myvariable_show, (void *)myvariable_store);

static int __init mymodule_init(void)
{
    int error = 0;
    pr_info("mymodule: initialised\n");
    mymodule = kobject_create_and_add("mymodule", kernel_kobj);
    if (!mymodule)
        return -ENOMEM;
    error = sysfs_create_file(mymodule, &myvariable_attribute.attr);
    if (error) {
        pr_info("failed to create the myvariable file in /sys/kernel/mymodule\n");
    }
    return error;
}

static void __exit mymodule_exit(void)
{
    pr_info("mymodule: Exit success\n");
    kobject_put(mymodule);
}

module_init(mymodule_init);
module_exit(mymodule_exit);
MODULE_LICENSE("GPL");
```

## Talking to Device files

So we are now able to create device files either in `/dev/` or in `/proc` or in `/sys/`.
Ok, the problem now is: we want to be able to write into a device file and read the response inside the same device file.
For example image a modem connetec to the serial port. We read/write to the device file representing the modem but how can we control the serial port itself? By using the function `ioctl`.

Every device has it's own `ioctl_read` and `iotcl_write`:

- whit the `ioctl_read` the device can read from the process interacting with it
- whit the `ioctl_write` the device can write information for the process to read

An example divided in three files

```c
/*
 * chardev2.c - Create an input/output character device
 */
#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h> /* Specifically, a module */
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/version.h>
#include <asm/errno.h>
#include "chardev.h"
#define SUCCESS 0
#define DEVICE_NAME "char_dev"
#define BUF_LEN 80

enum {
    CDEV_NOT_USED = 0,
    CDEV_EXCLUSIVE_OPEN = 1,
};

/* Is the device open right now? Used to prevent concurrent access into
 * the same device
 */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);
/* The message the device will give when asked */
static char message[BUF_LEN + 1];
static struct class *cls;

/* This is called whenever a process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
    pr_info("device_open(%p)\n", file);
    // This increments by 1 the number of devices using the module so it can't be removed by accident
    try_module_get(THIS_MODULE);
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    pr_info("device_release(%p,%p)\n", inode, file);
    // This lowers by 1 the number of devices using the module so it can't be removed by accident
    module_put(THIS_MODULE);
    return SUCCESS;
}

/* This function is called whenever a process which has already opened the device file attempts to read from it. */
static ssize_t device_read(struct file *file, /* see include/linux/fs.h*/
                           char __user *buffer, /* buffer to be filled  */
                           size_t length, /* length of the buffer */
                           loff_t *offset)
{
    /* Number of bytes actually written to the buffer */
    int bytes_read = 0;
    /* How far did the process reading the message get? Useful if the message
     * is larger than the size of the buffer we get to fill in device_read.
     */
    const char *message_ptr = message;
    if (!*(message_ptr + *offset)) { /* we are at the end of message */
        *offset = 0; /* reset the offset */
        return 0; /* signify end of file */
    }
    message_ptr += *offset;
    /* Actually put the data into the buffer */
    while (length && *message_ptr) {
        /* Because the buffer is in the user data segment, not the kernel
         * data segment, assignment would not work. Instead, we have to
         * use put_user which copies data from the kernel data segment to
         * the user data segment.
         */
        put_user(*(message_ptr++), buffer++);
        length--;
        bytes_read++;
    }
    pr_info("Read %d bytes, %ld left\n", bytes_read, length);
    *offset += bytes_read;
    /* Read functions are supposed to return the number of bytes actually
     * inserted into the buffer.
     */
    return bytes_read;
}

/* called when somebody tries to write into our device file. */
static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset)
{
    int i;
    pr_info("device_write(%p,%p,%ld)", file, buffer, length);
    // get_user(variable_to_stere_result, source_address_user_space), is used to get a simple variable from user space
    for (i = 0; i < length && i < BUF_LEN; i++)
        get_user(message[i], buffer + i);
    /* Again, return the number of input characters used. */
    return i;
}

/* This function is called whenever a process tries to do an ioctl on our device file. We get two extra parameters (additional to the inode and file structures, which all device functions get):
- the number of the ioctl called
- the parameter given to the ioctl function
If the ioctl is write or read/write (meaning output is returned to the calling process), the ioctl call returns the output of this function. */

static long device_ioctl(struct file *file, /* ditto */
             unsigned int ioctl_num, /* number and param for ioctl */
             unsigned long ioctl_param)
{
    int i;
    long ret = SUCCESS;

    /* We don't want to talk to two processes at the same time. */
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;

    /* Switch according to the ioctl called */
    switch (ioctl_num) {
        case IOCTL_SET_MSG: {
            /* Receive a pointer to a message (in user space) and set that to be the device's message. Get the parameter given to ioctl by the process.
            */
            char __user *tmp = (char __user *)ioctl_param;
            char ch;
            /* Find the length of the message */
            get_user(ch, tmp);
            for (i = 0; ch && i < BUF_LEN; i++, tmp++)
                get_user(ch, tmp);
            device_write(file, (char __user *)ioctl_param, i, NULL);
            break;
        }

        case IOCTL_GET_MSG: {
            loff_t offset = 0;
            /* Give the current message to the calling process - the parameter
            * we got is a pointer, fill it.
            */
            i = device_read(file, (char __user *)ioctl_param, 99, &offset);
            /* Put a zero at the end of the buffer, so it will be properly
            * terminated.
            */
            put_user('\0', (char __user *)ioctl_param + i);
            break;
        }
        case IOCTL_GET_NTH_BYTE:
            /* This ioctl is both input (ioctl_param) and output (the return
            * value of this function).
            */
            ret = (long)message[ioctl_param];
            break;
    }

    /* We're now ready for our next caller */
    atomic_set(&already_open, CDEV_NOT_USED);
    return ret;
}

/* Module Declarations */
/* This structure will hold the functions to be called when a process does
 * something to the device we created. Since a pointer to this structure
 * is kept in the devices table, it can't be local to init_module. NULL is
 * for unimplemented functions.
 */
static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .open = device_open,
    .release = device_release, /* a.k.a. close */
};

/* Initialize the module - Register the character device */
static int __init chardev2_init(void)
{
    /* Register the character device (atleast try) */
    int ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);

    /* Negative values signify an error */
    if (ret_val < 0) {
        pr_alert("%s failed with %d\n",
                 "Sorry, registering the character device ", ret_val);
        return ret_val;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cls = class_create(DEVICE_FILE_NAME);
#else
    cls = class_create(THIS_MODULE, DEVICE_FILE_NAME);
#endif
    device_create(cls, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_FILE_NAME);
    pr_info("Device created on /dev/%s\n", DEVICE_FILE_NAME);
    return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
static void __exit chardev2_exit(void)
{
    device_destroy(cls, MKDEV(MAJOR_NUM, 0));
    class_destroy(cls);
    /* Unregister the device */
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

module_init(chardev2_init);
module_exit(chardev2_exit);
MODULE_LICENSE("GPL");
```

```c
/*
 * chardev.h - the header file with the ioctl definitions.
 *
 * The declarations here have to be in a header file, because they need
 * to be known both to the kernel module (in chardev2.c) and the process
 * calling ioctl() (in userspace_ioctl.c).
 */
#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>
/* The major device number. We can not rely on dynamic registration
 * any more, because ioctls need to know it.
 */
#define MAJOR_NUM 100

/* Set the message of the device driver */
#define IOCTL_SET_MSG _IOW(MAJOR_NUM, 0, char *)
/* _IOW means that we are creating an ioctl command number for passing information from a user process to the kernel module.
The first arguments, MAJOR_NUM, is the major device number we are using.
The second argument is the number of the command (there could be several
with different meanings).
The third argument is the type we want to get from the process to the kernel.
 */

/* Get the message of the device driver */
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)
/* This IOCTL is used for output, to get the message of the device driver. However, we still need the buffer to place the message in to be input, as it is allocated by the process. */
/* Get the n'th byte of the message */
#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)
/* The IOCTL is used for both input and output. It receives from the user a number, n, and returns message[n]. */

/* The name of the device file */
#define DEVICE_FILE_NAME "char_dev"
#define DEVICE_PATH "/dev/char_dev"
#endif
```

```c
/*  userspace_ioctl.c - the process to use ioctl's to control the kernel module
 *  Until now we could have used cat for input and output.  But now
 *  we need to do ioctl's, which require writing our own process.
 */

/* device specifics, such as ioctl numbers and the
 * major device file. */
#include "../chardev.h"
#include <stdio.h> /* standard I/O */
#include <fcntl.h> /* open */
#include <unistd.h> /* close */
#include <stdlib.h> /* exit */
#include <sys/ioctl.h> /* ioctl */

/* Functions for the ioctl calls */
int ioctl_set_msg(int file_desc, char *message)
{
    int ret_val;
    ret_val = ioctl(file_desc, IOCTL_SET_MSG, message);
    if (ret_val < 0) {
        printf("ioctl_set_msg failed:%d\n", ret_val);
    }
    return ret_val;
}

int ioctl_get_msg(int file_desc)
{
    int ret_val;
    char message[100] = { 0 };

    /* Warning - this is dangerous because we don't tell
   * the kernel how far it's allowed to write, so it
   * might overflow the buffer. In a real production
   * program, we would have used two ioctls - one to tell
   * the kernel the buffer length and another to give
   * it the buffer to fill
   */
    ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);
    if (ret_val < 0) {
        printf("ioctl_get_msg failed:%d\n", ret_val);
    }
    printf("get_msg message:%s", message);
    return ret_val;
}

int ioctl_get_nth_byte(int file_desc)
{
    int i, c;
    printf("get_nth_byte message:");
    i = 0;
    do {
        c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++);
        if (c < 0) {
            printf("\nioctl_get_nth_byte failed at the %d'th byte:\n", i);
            return c;
        }
        putchar(c);
    } while (c != 0);
    return 0;
}

/* Main - Call the ioctl functions */
int main(void)
{
    int file_desc, ret_val;
    char *msg = "Message passed by ioctl\n";
    file_desc = open(DEVICE_PATH, O_RDWR);
    if (file_desc < 0) {
        printf("Can't open device file: %s, error:%d\n", DEVICE_PATH,
               file_desc);
        exit(EXIT_FAILURE);
    }

    ret_val = ioctl_set_msg(file_desc, msg);
    if (ret_val)
        goto error;
    ret_val = ioctl_get_nth_byte(file_desc);
    if (ret_val)
        goto error;
    ret_val = ioctl_get_msg(file_desc);
    if (ret_val)
        goto error;
    close(file_desc);
    return 0;

error:
    close(file_desc);
    exit(EXIT_FAILURE);
}
```

Summary: ioctl call takes as parameters:

- an open file descriptor
  -a request code number
- an untyped pointer to data (either going to the driver, coming back from the driver, or both).

The kernel generally dispatches an ioctl call straight to the device driver, which can interpret the request number and data in whatever way required. The writers of each driver document request numbers for that particular driver and provide them as constants in a header file.

## Blocking processes and threads

### Sleep

If a process requests something to a kernel module that is busy, the kernel module can put it to sleep. If you want to see how, read this chapter in the book.

### Completions

Are another way of achiving the same thing as sleep but whout sleeping, once again read the book if interested.

## Avoiding collisions and deadlocks

We can use different methods in order to avoid that multiple processes try to access the same memory.

### Mutex

```c
/* example_mutex.c */
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>

static DEFINE_MUTEX(mymutex);

static int __init example_mutex_init(void)
{
    int ret;
    pr_info("example_mutex init\n");
    ret = mutex_trylock(&mymutex);
    if (ret != 0) {
        pr_info("mutex is locked\n");
        if (mutex_is_locked(&mymutex) == 0)
            pr_info("The mutex failed to lock!\n");
        mutex_unlock(&mymutex);
        pr_info("mutex is unlocked\n");
    } else
        pr_info("Failed to lock\n");
      return 0;
}

static void __exit example_mutex_exit(void)
{
    pr_info("example_mutex exit\n");
}
module_init(example_mutex_init);
module_exit(example_mutex_exit);

MODULE_DESCRIPTION("Mutex example");
MODULE_LICENSE("GPL");
```

## Interrupt handlers

Everyhting seen so far was always in response to a process asking for it (using `ioclt`, by opening a specific file or by using a syscall). But the kernel doesn't only respond to process but it also talks with the hw.

So we have the CPU that talks to the hw but the hw is not always ready, we need a way to issue commands and than read the response in a very specific time frame. For this reason we have **interrupts**:

- short interrupts: it will last a short period of thime and in the meanwhile the machine will be blocked and no other interrupts handled
- long interrupts: will take longer and in the meantime other interrupts can occour -but not from the same device-

Example

```c
/*
 * bottomhalf.c - Top and bottom half interrupt handling
 * Based upon the RPi example by Stefan Wendler (devnull@kaltpost.de)
 * from:
 *    https://github.com/wendlers/rpi-kmod-samples
 * Press one button to turn on an LED and another to turn it off
 */
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/init.h>

/* Macro DECLARE_TASKLET_OLD exists for compatibility.
 * See https://lwn.net/Articles/830964/
 */
#ifndef DECLARE_TASKLET_OLD
#define DECLARE_TASKLET_OLD(arg1, arg2) DECLARE_TASKLET(arg1, arg2, 0L)
#endif

static int button_irqs[] = { -1, -1 };

/* Define GPIOs for LEDs.
 * TODO: Change the numbers for the GPIO on your board.
 */
static struct gpio leds[] = { { 4, GPIOF_OUT_INIT_LOW, "LED 1" } };

/* Define GPIOs for BUTTONS
 * TODO: Change the numbers for the GPIO on your board.
 */
static struct gpio buttons[] = {
    { 17, GPIOF_IN, "LED 1 ON BUTTON" },
    { 18, GPIOF_IN, "LED 1 OFF BUTTON" },
};

/* Tasklet containing some non-trivial amount of processing */
static void bottomhalf_tasklet_fn(unsigned long data)
{
    pr_info("Bottom half tasklet starts\n");
    /* do something which takes a while */
    mdelay(500);
    pr_info("Bottom half tasklet ends\n");
}

static DECLARE_TASKLET_OLD(buttontask,bottomhalf_tasklet_fn);

/* interrupt function triggered when a button is pressed */
static irqreturn_t button_isr(int irq, void *data)
{
    /* Do something quickly right now */
    if (irq == button_irqs[0] && !gpio_get_value(leds[0].gpio))
        gpio_set_value(leds[0].gpio, 1);
    else if (irq == button_irqs[1] && gpio_get_value(leds[0].gpio))
        gpio_set_value(leds[0].gpio, 0);

    /* Do the rest at leisure via the scheduler */
    // We are saying, when you want execute bottomhalff_tasklet_fn
    tasklet_schedule(&buttontask);
    return IRQ_HANDLED;
}

static int __init bottomhalf_init(void)
{
    int ret = 0;
    pr_info("%s\n", __func__);

    /* register LED gpios */
    ret = gpio_request_array(leds, ARRAY_SIZE(leds));

    if (ret) {
        pr_err("Unable to request GPIOs for LEDs: %d\n", ret);
        return ret;
    }

    /* register BUTTON gpios */
    ret = gpio_request_array(buttons, ARRAY_SIZE(buttons));

    if (ret) {
        pr_err("Unable to request GPIOs for BUTTONs: %d\n", ret);
        goto fail1;
    }

    pr_info("Current button1 value: %d\n", gpio_get_value(buttons[0].gpio));
    ret = gpio_to_irq(buttons[0].gpio);

    if (ret < 0) {
        pr_err("Unable to request IRQ: %d\n", ret);
        goto fail2;
    }

    button_irqs[0] = ret;
    pr_info("Successfully requested BUTTON1 IRQ # %d\n", button_irqs[0]);
    ret = request_irq(button_irqs[0], button_isr,
                      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                      "gpiomod#button1", NULL);

    if (ret) {
        pr_err("Unable to request IRQ: %d\n", ret);
        goto fail2;
    }
    ret = gpio_to_irq(buttons[1].gpio);
    if (ret < 0) {
        pr_err("Unable to request IRQ: %d\n", ret);
        goto fail2;
    }
    button_irqs[1] = ret;
    pr_info("Successfully requested BUTTON2 IRQ # %d\n", button_irqs[1]);

    ret = request_irq(button_irqs[1], button_isr,
                      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                      "gpiomod#button2", NULL);
    if (ret) {
        pr_err("Unable to request IRQ: %d\n", ret);
        goto fail3;
    }
    return 0;

/* cleanup what has been setup so far */
fail3:
    free_irq(button_irqs[0], NULL);
fail2:
    gpio_free_array(buttons, ARRAY_SIZE(leds));
fail1:
    gpio_free_array(leds, ARRAY_SIZE(leds));
    return ret;
}

static void __exit bottomhalf_exit(void)
{
    int i;
    pr_info("%s\n", __func__);
    /* free irqs */
    free_irq(button_irqs[0], NULL);
    free_irq(button_irqs[1], NULL);

    /* turn all LEDs off */
    for (i = 0; i < ARRAY_SIZE(leds); i++)
        gpio_set_value(leds[i].gpio, 0);

    /* unregister */
    gpio_free_array(leds, ARRAY_SIZE(leds));
    gpio_free_array(buttons, ARRAY_SIZE(buttons));
}
module_init(bottomhalf_init);
module_exit(bottomhalf_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Interrupt with top and bottom half");
```

But what is this `bottom_half`? The interrupt handling is splitted in two parts:

- the first one executes right away and masks the interrupt line
- the second part (bottom_half) handles the heavy work deferred from an interrupt handler

The way to implement this is to call `request_irq()` to get the interrupt handler called when the relevant IRQ is received.

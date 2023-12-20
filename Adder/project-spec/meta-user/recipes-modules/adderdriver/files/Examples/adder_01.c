/**
 * In this file we try to interface with a driver a static
 * "virtual adder" inside the C code but it gives a clear
 * picture of how the driver works
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define DRIVER_NAME  "axi4adder"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("ivanpiri");
MODULE_VERSION("1.0");

/*
Once we have the driver loaded you need to get the major number
1. cat /proc/devices
2. search for your device
3. mknod /dev/nomechevuoidarealfiel c major_number 0
4. echo 1 > /dev/nomechehaidatoalfile
5. cat /dev/nomechehaidatoalfile
*/

// Struct with informations the driver needs to run
static struct {
	// Variabile che salva major/minor number
	dev_t devnum;
	// Variabile che contiene il char device che sara' registrato nel kernel
	struct cdev cdev;
	// Non voglio scrivere nei registri fisici, facciamo finta di avere
	// dei registri "virtuali"
	unsigned int reg0;
	unsigned int reg1;
	unsigned int res;
} axi4adder_data;

static void axi4adder_setregs(unsigned int reg0, unsigned int reg1)
{
	axi4adder_data.reg0 = reg0;
	axi4adder_data.reg1 = reg1;
	axi4adder_data.res = axi4adder_data.reg0 + axi4adder_data.reg1;
	pr_info("*************setregs*******************");
	pr_info("REG0: %d | REG1: %d  | RES: %d\n", reg0, reg1, axi4adder_data.res);
}

// Rimane questo problema
// La read viene chiamata per passare res all'utente
static ssize_t axi4adder_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	if (*ppos > 0)
		return 0;

    // Dimensiona il buffer in base alla lunghezza massima della stringa che vuoi creare
    char message_buffer[100];

    // Formatta la stringa con variabili
	int length = 0;
    length = snprintf(message_buffer, sizeof(message_buffer),
                          "Reg0: %d | Reg1: %d | Res: %d \n",
                          axi4adder_data.reg0, axi4adder_data.reg1, axi4adder_data.res);

    // Verifica se la formattazione è avvenuta correttamente
    if (length >= sizeof(message_buffer)) {
        // Errore: la stringa è troppo lunga per il buffer
        pr_err("Errore di formattazione: stringa troppo lunga\n");
        return -EFAULT;
    }

    if (copy_to_user(buf, message_buffer, length))
		return -EFAULT;

	
	length = sizeof(message_buffer);

	*ppos += length;
	pr_info("*PPOS dopo l'incremento vale: %lld \n", *ppos);
	return length;
}

static ssize_t axi4adder_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{		
	uint reg0 = 0;
	uint reg1 = 0;
	uint buffer[2];

    // printf "%b" "\\xFF\\x00\\x00\\x00\\xFF\\x00\\x00\\x00" > /dev/axi

    if (copy_from_user(buffer, buf, 8))
        return -EFAULT;

    reg0 = buffer[0];
    reg1 = buffer[1];
    axi4adder_setregs(reg0, reg1);

    return count;

}

static const struct file_operations axi4adder_fops = {
	.owner = THIS_MODULE,
	.write = axi4adder_write,
	.read = axi4adder_read,
};

static int __init axi4adder_init(void)
{
	int result;

	// Allocate major e minor number
	result = alloc_chrdev_region(&axi4adder_data.devnum, 0, 1, DRIVER_NAME);
	if (result) {
		pr_err("%s: Failed to allocate device number!\n", DRIVER_NAME);
		return result;
	}
	
	// Init della struct che rappresenta il char device
	cdev_init(&axi4adder_data.cdev, &axi4adder_fops);
	
	// Registrazione nel kernel
	result = cdev_add(&axi4adder_data.cdev, axi4adder_data.devnum, 1);
	if (result) {
		pr_err("%s: Char device registration failed!\n", DRIVER_NAME);
		unregister_chrdev_region(axi4adder_data.devnum, 1);
		return result;
	}

	axi4adder_setregs(0,0);

	pr_info("%s: initialized.\n", DRIVER_NAME);
	return 0;
}

static void __exit axi4adder_exit(void)
{
	pr_info("Called the exit function \n");
	cdev_del(&axi4adder_data.cdev);
	unregister_chrdev_region(axi4adder_data.devnum, 1);
	pr_info("%s: exiting.\n", DRIVER_NAME);
}

module_init(axi4adder_init);
module_exit(axi4adder_exit);

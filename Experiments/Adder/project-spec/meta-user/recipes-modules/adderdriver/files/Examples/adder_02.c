/**
 * In this version we are basically doing the same thing as adder_1.c but
 * we now access the physical device inside the FPGA with the addresses
 * hardcoded
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>

/*
Una possibile alternativa a tutti quegli accrocchi in read/write poteva essere:
1. definisci un buffer statico di una certa grandezza
2. copy tutto quello che l'utente ti manda dentro il buffer
3. in posizione buf[len] (dove len sarebbe il parametro len della write) scrivi "0"
4. Quando l'utente fa read semplicemente gli spari indietro il buffer

In teoria come altre funzioni avevi:
- access_ok(): controlla la validita' del puntatore alla user space memory
- get_user(): prende una variabile semplice dallo user space
- put_user(); manda una variabile semplice allo user space
- clear_user(): tutti zero in un blocco nello user space
- copy_to_user(): lo sai
- copy_from_user(): lo sai
- strlen_user(): ti da la size di uno string buffer dentro lo user space
- strncpy_from_user(): copia una stringa da user space in kernel space
*/

#define DRIVER_NAME  "axi4adder"

#define ADDER_BASE	0x43C00000
#define ADDER_SIZE	12

#define ADDER_REG0	0
#define ADDER_REG1	4
#define ADDER_RES   8

static struct {
	dev_t devnum;
	struct cdev cdev;
	unsigned int res;
	void __iomem *regbase;
} axi4adder_data;

static void axi4adder_setregs(unsigned int reg0, unsigned int reg1)
{
    // Scrivo i valori
    writel(reg0, axi4adder_data.regbase + ADDER_REG0);
    writel(reg1, axi4adder_data.regbase + ADDER_REG1);
    axi4adder_data.res = readl(axi4adder_data.regbase + ADDER_RES);
}

static ssize_t axi4adder_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	if (*ppos > 0)
		return 0;

    // Dimensiona il buffer in base alla lunghezza massima della stringa che vuoi creare
    char message_buffer[100];

    // Formatta la stringa con variabili
	int length = 0;
    /*
    length = snprintf(message_buffer, sizeof(message_buffer),
                        "%d + %d = %d\n",
                        readl(axi4adder_data.regbase + ADDER_REG0),
                        readl(axi4adder_data.regbase + ADDER_REG1),
                        readl(axi4adder_data.regbase + ADDER_RES));
    */
   length = snprintf(message_buffer, sizeof(message_buffer),
                    "Risultato: %d \n",
                    axi4adder_data.res);

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
	int result = 0;

    // Chiediamo una parte di memoria per usarla come MMIO
	if (!request_mem_region(ADDER_BASE, ADDER_SIZE, DRIVER_NAME)) {
		pr_err("%s: Error requesting I/O!\n", DRIVER_NAME);
		result = -EBUSY;
		goto ret_err_request_mem_region;
	}

    // Appena otteniamo l'area di memoria la rimappiamo 
	axi4adder_data.regbase = ioremap(ADDER_BASE, ADDER_SIZE);
	if (!axi4adder_data.regbase) {
		pr_err("%s: Error mapping I/O!\n", DRIVER_NAME);
		result = -ENOMEM;
		goto err_ioremap;
	}

    // Come prima abbiamo le syscall per registrare un character device
	result = alloc_chrdev_region(&axi4adder_data.devnum, 0, 1, DRIVER_NAME);
	if (result) {
		pr_err("%s: Failed to allocate device number!\n", DRIVER_NAME);
		goto ret_err_alloc_chrdev_region;
	}

	cdev_init(&axi4adder_data.cdev, &axi4adder_fops);

	result = cdev_add(&axi4adder_data.cdev, axi4adder_data.devnum, 1);
	if (result) {
		pr_err("%s: Char device registration failed!\n", DRIVER_NAME);
		goto ret_err_cdev_add;
	}

	axi4adder_setregs(0, 0);

	pr_info("%s: initialized.\n", DRIVER_NAME);
	goto ret_ok;

ret_err_cdev_add:
	unregister_chrdev_region(axi4adder_data.devnum, 1);
ret_err_alloc_chrdev_region:
	iounmap(axi4adder_data.regbase);
err_ioremap:
	release_mem_region(ADDER_BASE, ADDER_SIZE);
ret_err_request_mem_region:
ret_ok:
	return result;
}

static void __exit axi4adder_exit(void)
{
	cdev_del(&axi4adder_data.cdev);
	unregister_chrdev_region(axi4adder_data.devnum, 1);
	iounmap(axi4adder_data.regbase);
	release_mem_region(ADDER_BASE, ADDER_SIZE);
	pr_info("%s: exiting.\n", DRIVER_NAME);
}

module_init(axi4adder_init);
module_exit(axi4adder_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Piri");
MODULE_VERSION("1.0");

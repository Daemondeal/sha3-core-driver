# Ketchup_driver

For our keccak peripheral we implemented a **character driver** that manages write and read operation from and to the custom logic block.

Since the peripheral is contained into the FPGA part of the Pynq-Z2 board, we implemented it as a **platform device driver** (so it doesn't support discoverability).

## Peripherals setup

> This is a brief overview. For detailed information on the hardware's inner workings, please refer to the dedicated folders for each peripheral.

Inside the Pynq-Z2 board we managed to fit 4 completely identical keccak-peripherals.

As shown in the device tree file (screenshot attached below), all peripherals are integrated into the device tree structure under the _amba-pl_ node.

> **Note**: add the screenshot

The same structure is reflected in the _sysfs_ file system: all peripheral-related files are located at the specified path (screenshot attached below).

> **Note**: add the screenshot

In terms of the driver, the steps required for successful communication with the peripheral differ depending on the operation requested. There are two cases to consider:

- a **write** operation
- a **read** operation

### Write operation

When a user application in user space intends to write data to our peripheral, it simply needs to open the device file `/dev/ketchup_driver` and write the data.

At a high level, when the device file is opened, the peripheral is initialized with the appropriate hash size requested by the user. Then, the data that the user wants to send to the peripheral is copied into the kernel space using the `copy_from_user()` function, and then sent to the input register of the peripheral in 4-byte chunks using an internal buffer.

The peripheral remains "idle" until an additional write is performed on the same file descriptor.

### Read operation

When a user process aims to read from the peripheral, there must be an already open file descriptor for the `/dev/ketchup_driver` file.

During the read attempt, the driver transmits a "last input" command to the peripheral, as well as any remaining data from the internal buffer (if available). If no data remains in the buffer, random data is transmitted. This isn't an issue because, when writing to the peripheral's input register, we include a field indicating the length of the input we're sending.

Upon sending the last input, we await the completion of the hashing operation by polling the peripheral's status register until it reads 1.

As soon as the result is available, we read it from the 15 output registers. The number of registers we read from depends on the hash that has been computed.

## Interfacing with the OS

The way our driver interfaces with the OS can be summarized with this diagram

![diagram](../Tutorials/Resources/petalinux/driver_interconnections.png)

When a user application writes to the device file, the operating system (OS) identifies the associated driver for that specific device file by using the major and minor numbers of that file. Subsequently, the OS can invoke the specific functions we have defined for our device.

In the diagram, we have also indicated the various system calls, or _syscalls_, provided by the Linux kernel that we utilized to register our driver with the OS.

## Interacting with the driver

[qua dentro attributi che esponi, ioctl e custom library]

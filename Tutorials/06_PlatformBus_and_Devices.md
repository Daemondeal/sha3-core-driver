# Platform bus and devices

**Platform device** == device that doesn't support discoverability.
A bus is a collection of wires that enables the transfer of data or addresses.
**Platform bus** == pseudo-bus (virtual bus) representing all non-disoverable hardware.

Every device has it's own resources and configuration that the os needs to know.
How the os find's out? With **enumeration**.
**Enumeration** is a process through which the operating system can inquire and receive information, such as the type of the device, the manufacturer, device configuration, and all of the devices connected to a given bus. 
Than once the information is retrieved the OS can auto-load the correct driver.

## Information needed

The device information may include memory or I/O mapped base address and range information, IRQ number on which the device issues interrupts to the processor, device identification information, DMA channel information, device address, pin configuration, power or voltage parameters, and other device specific data. 

## Resources

- [Implementation details about buses and drivers](http://www.makelinux.net/ldd3/chp-14-sect-4.shtml)
- [Platform devices and platform drivers](https://kerneltweaks.wordpress.com/2014/03/30/platform-device-and-platform-driver-linux/)
- Linux kernel in linux/platform_device.h
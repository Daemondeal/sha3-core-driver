# Ketchup Library

This is the code for the userspace library needed to use the peripheral from a C program.

## Reference

> TODO

## OpenSSL

This library supports also an OpenSSL target other than the hardware one. This is mostly just for testing that the hardware is actually correct, and to compare performance with an identical interface.

To make building for the Petalinux target easier, we actually statically link against OpenSSL. Inside the `./openssl` folder, there is everything needed to do so. Do note that this means that the resulting binary will be pretty large, so it's going to be very slow to transfer it over UART. 

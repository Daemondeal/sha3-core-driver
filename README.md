# Embedded Linux with Custom Hardware

In this project we are developing a crypto core that accelerates the computation of the SHA (Secure Hash Algorithm) and concurrently creating the driver for the integration of the peripheral on the PYNQ-Z2 board running a Linux image created using Petalinux.

## The Peripheral

The peripheral implements the Keccak hash function.
Keccak is a family of cryptographic hash functions designed by Guido Bertoni, Joan Daemen, Michael Peeters, and Gilles Van Assche. This family of hash functions was selected as the winner of the "SHA-3" (Secure Hash Algorithm 3) competition organized by the National Institute of Standards and Technology (NIST) of the United States in 2012.
We have found the implementation of this github repository and we had to firstly understand it and than adapt it to work inside our board..
Additional details can be found within the KeccakHardware folder.

> Note: add the repo link

## The Driver

The driver for the peripheral is a simple platform driver that exposes the device within /dev/ as a character device. It also exposes various attributes to the user within /sys/ using sysfs. Further details can be found in the .. folder.

> Note: add the folder name

## Project Status

> TODO

## Contributing

Each folder in this repository is a Petalinux project. Simply navigate to it in the terminal and run petalinux-build, and the project will be built.

> Note: Before committing, run the command petalinux-build -x mrproper to avoid uploading a large folder.
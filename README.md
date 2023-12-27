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

Inside this repository we can find:

- Adder folder: inside this folder you can find the driver and the peripheral for the adder shown in the tutorial
- KeccakHardware: inside this folder there is the Keccak peripheral
- LedExampleProject: inside this folder you can find a pwn peripheral and a driver that handles the RGB led's of the ZYNQ board
- Tutorials: here you can find a collection of a variety of tutorials written by us regarding all the steps necessary to work with the environment and also some "deep" dives into the different topics that must be understood in order to develop a driver

We tried to version control directly the entire petalinux project folder, but the folder is too big and once you try to build it, Yocto starts the download of all the dependencies once again (even if in theory they should be already there) and it takes too long.

> Inside of each folder there is a README.md, refer to it for more details about the contents of the different folders

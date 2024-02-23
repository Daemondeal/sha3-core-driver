# Embedded Linux with Custom Hardware

In this project we are developing a crypto core that accelerates the computation of the SHA (Secure Hash Algorithm) and concurrently creating the driver for the integration of the peripheral on the PYNQ-Z2 board running a Linux image created using Petalinux.

## Project Status

The project status is as follows: (Last updated by Pietro on the 2024-02-23)

- Peripheral: Now complete and working, it also has an automatic test suite that generates test vectors with python;
- HardwarePlatform: Complete and working, it also has some tests with Vitis. While they aren't fully automated or very comprehensive, i feel it's unnecessary since the platform will be also tested for the driver;
- Driver & Userspace: The driver is also complete and working, and it now implements an automatic test suit with a format similar to the [official NIST tests](https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing). There is also code for benchmarking this implementation with the OpenSSL one; the only thing missing is to add some multiprocessing demo.

The main thing missing as of now is documentation, a lot of documentation. This includes:

- Driver: We should write a full specification of what functionalities the driver offers, and of how to use it. Another thing we should write about is driver internals, the concurrency primitives we used, platform drivers and that kinda stuff. We should also document a little our configuration of petalinux, and maybe we should say something about Petalinux itself;
- Userspace: We should also write a better guide on how to use the library, and especially how to compile it.

Also, we need to add a table of contents, and update this readme properly before delivering.

## The Peripheral

The peripheral implements the Keccak hash function.
Keccak is a family of cryptographic hash functions designed by Guido Bertoni, Joan Daemen, Michael Peeters, and Gilles Van Assche. This family of hash functions was selected as the winner of the "SHA-3" (Secure Hash Algorithm 3) competition organized by the National Institute of Standards and Technology (NIST) of the United States in 2012.
We started from a core that implemented the algorithm (from opencores.com, [link to repo](https://github.com/freecores/sha3)), and adapted it to our needs.
Additional details can be found within the `Peripheral` and `HardwarePlatform` folders.

## The Driver

The driver for the peripheral is a simple platform driver that exposes the device within /dev/ as a character device. It also exposes various attributes to the user within /sys/ using sysfs. Further details can be found in the `Petalinux` folder.

> Note: add a README inside the `Petalinux` folder.

## Contributing

Inside this repository we can find:

- Peripheral: inside this folder there is the verilog code for the SHA3 peripheral;
- HardwarePlatform: here you can find:
  - The `.xsa` for the final hardware platform;
  - Some sample code to be used within Vitis to test out the peripheral's functionalities in a bare metal environment.
- Petalinux: here you can find the Petalinux project, alongside the description of the driver;
- Userspace: here you can find reference implementations for the desired algorithm, alongside some demos to be run within the Petalinux linux environment to check the driver's correctness;
- Tutorials: here you can find a collection of a variety of tutorials written by us regarding all the steps necessary to work with the environment and also some "deep" dives into the different topics that must be understood in order to develop a driver;
- Experiments: inside this folder, you can find both the driver and peripheral for the adder shown in the tutorial, as well as a sample project that turns some led on from linux.

> Inside of each folder there is a README.md, refer to it for more details about the contents of the different folders

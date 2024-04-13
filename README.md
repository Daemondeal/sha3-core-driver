# Ketchup Driver

This project consisted in using an Hardware implementation for the cryptographic family of functions SHA3, and writing both a bus interface for the AXI4-Lite bus protocol and a linux driver for the peripheral. The reference board used for the project was the PYNQ-Z2.


## Demo of the project running in the PYNQ-Z2

https://github.com/Daemondeal/sha3-core-driver/assets/16180334/994b9b0d-ea1f-476f-891b-4f0e79983aee


All the steps needed to replicate this demo are detailed in the [DEMO.md](./DEMO.md) file.

## The Peripheral

The peripheral implements the SHA3-512, SHA3-384, SHA3-256 and SHA3-224 cryptographic hash functions, as defined by [FIPS 202](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.202.pdf).
The functions that were implemented are part of the Keccak family, which were designed by Guido Bertoni, Joan Daemen, Michael Peeters, and Gilles Van Assche. This family of hash functions was selected as the winner of the "SHA-3" (Secure Hash Algorithm 3) competition organized by the National Institute of Standards and Technology (NIST) of the United States in 2012.
The base for the implemetation is a core that implemented the original Keccak algorithm proposal (from opencores.com, [link to repo](https://github.com/freecores/sha3)), and adapted it both to be configurable, and to update it to the final published standard.
Additional details about the core can be found within the `Peripheral` folder's [README](./Peripheral/README.md), while additional details about the hardware implementation and software interface to the peripheral can be found inside the `HardwarePlatform` folder's [README](./HardwarePlatform/README.md).

## The Driver

The driver for the peripheral is a simple platform driver that exposes the device within /dev/ as a character device. It also exposes various attributes to the user within /sys/ using sysfs. The `Petalinux` folder contains a petalinux project, and further details about the project and the driver can be found in its [README](./Petalinux/README.md) file.

## The Library

The driver offers also a small userspace library, which can be built both with an hardware backend and with an OpenSSL backend. Further information can be found inside the `Userspace/KetchupLibrary` folder's [README](./Userspace/KetchupLibrary/README.md).

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

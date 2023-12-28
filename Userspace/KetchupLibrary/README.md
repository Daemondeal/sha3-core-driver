# Ketchup Library

This is the code for the userspace library needed to use the peripheral from a C program.

## Reference

> TODO

## Sample Code

There is a sample usage in `example.c`. There are three supported targets for this example:
- You can run it locally, using OpenSSL as a backend. To build the executable this way, run just `make` or `make compile`
- You can run it on the PYNQ under Petalinux with the OpenSSL backend. To do that, run `make arm_openssl`
- You can run it on the PYNQ using the hardware peripheral. To do it, run `make arm`

The local OpenSSL backend depends on OpenSSL. To install its libraries under ubuntu, run:
```sh
sudo apt install libssl-dev
```

Note that the `arm_openssl` target statically links OpenSSL, so expect the resulting binary to be somewhat large, making any UART transfer very slow.

> Note: `make arm` currently not supported

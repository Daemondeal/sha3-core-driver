# Ketchup Library

This is the code for the userspace library that facilitates the peripheral's usage from a C program.

## Reference

To make an hash, first you need to call one of the init functions:
```C
kc_error kc_sha3_512_init(kc_sha3_context *context);
kc_error kc_sha3_384_init(kc_sha3_context *context);
kc_error kc_sha3_256_init(kc_sha3_context *context);
kc_error kc_sha3_224_init(kc_sha3_context *context);
```

They all give a `kc_sha3_context` outparam, which describes the current hashing context.

Once you have a context, to give it some data to hash you use the `kc_sha3_update` function:
```C
void kc_sha3_update(kc_sha3_context *context, void const *new_data, uint32_t new_data_length);
```

You can call this function repeatedly to update the current hash data. When you have given it all the data you need, to extract the digest call the function:
```C
void kc_sha3_final(kc_sha3_context *context, uint8_t *digest, uint32_t *digest_length);
```
The resulting `digest_length` is guaranteed to be less than `KC_MAX_MD_SIZE`, so allocating a digest array of that size is always safe. After `kc_sha3_final` is called, the context is automatically reset, so you can start a new hash right away.

At the end, free the context with:
```C
kc_error kc_sha3_close(kc_sha3_context *context);
```
Note that since only one process can have access to the peripheral at a time, it is extremely important to call this function when you're done hashing, especially if you're going to do something else with the program afterwards.

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

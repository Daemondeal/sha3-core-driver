# OpenSSL Reference

This is a test project meant to serve as a reference to test the correct functionality of the peripheral. To run this code, you must install the OpenSSL libs to your system. In Ubuntu, you can do that by running:
```sh
sudo apt install libssl-dev
```

## Building for ARM Linux

This can be also built for ARM Linux by running `make arm`. Note that the resulting binary statically links OpenSSL, so it's going to be pretty large and slow to transfer over UART.

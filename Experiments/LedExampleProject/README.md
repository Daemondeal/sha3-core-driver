# Led Example Project

This is an example project made to test the whole driver-making workflow. Its aim is to write a device driver for PetaLinux that can turn on both the RGB LEDs available on the board, and also choose their color.

The folder structure here is:
- `LedLinux`: contains the PetaLinux project, and the sources for the LED driver.

## PetaLinux Project

To run the project, just go inside the `LedPynq` folder, run `petalinux-boot` and wait for something like ~15 minutes. Once you're done, you can run the project on your Pynq board by running:
```bash
petalinux-boot --jtag --kernel
```

If you want to send the resulting module to the Pynq, just run:
```bash
bash ./copy_module.sh
```
And it will `uuencode` the module inside ledmodule.enc, within the project folder.
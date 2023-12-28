# Keccak Hardware

This folder is related to the hardware part of the Project. Under the Vitis folder, there is some test code for the peripheral. The hardware platform is defined by the `KetchupPlatform.xsa` file.

## KetchupPlatform Specification

This is an hardware platform with five different Ketchup Peripherals implemented, all identical. The peripherals and their starting addresses are thus defined:
|Index|HashSize|BaseAddress|
|-|-|-|
|0|`0x43C00000`|
|1|`0x43C10000`|
|2|`0x43C20000`|
|3|`0x43C30000`|

## Peripheral Specification

The Ketchup Peripheral has 20 registers, all starting from a common base address. Those registers are:
|Name|Offset|
|-|-|
|`CONTROL` |`0x00`|
|`STATUS`  |`0x04`|
|`INPUT`   |`0x08`|
|`COMMAND` |`0x0C`|
|`OUTPUT0` |`0x10`|
|`OUTPUT1` |`0x14`|
|`OUTPUT2` |`0x18`|
|`OUTPUT3` |`0x1C`|
|`OUTPUT4` |`0x20`|
|`OUTPUT5` |`0x24`|
|`OUTPUT6` |`0x28`|
|`OUTPUT7` |`0x2C`|
|`OUTPUT8` |`0x30`|
|`OUTPUT9` |`0x34`|
|`OUTPUT10`|`0x38`|
|`OUTPUT11`|`0x3C`|
|`OUTPUT12`|`0x40`|
|`OUTPUT13`|`0x44`|
|`OUTPUT14`|`0x48`|
|`OUTPUT15`|`0x4C`|

It can output SHA3 hashes of varying sizes, based on the appropriate bits in the control register.

## Control Register

The control registers is thus defined:
|Range|Purpose|
|-|-|
|`1:0`|Number of bits to be transmitted. Ignored if bit 2 is zero|
|`2`|Is the next write to `INPUT` the last for this stream?|
|`3`|Reserved|
|`5:4`|Size of output hash. Accepted values are: 00 for 512, 01 for 384, 10 for 256 and 11 for 224|
|`31:3`|Reserved|

NOTE: never change the value of bits 5 and 4 after having given the first bytes of the hash. The peripheral's behaviour is unspecified if you do so.

## Status Register

The status register is thus defined:
|Range|Purpose|
|-|-|
|`0`|Output Ready|
|`31:1`|Reserved|

Note that it is a read-only register.

## Input Register

This register contains the input for the hash function. Note that you need to write this register to send any data to the peripheral, even if you wish to send 0 bytes.

If in the control register the number of bytes to transmit is less than 4, the least significant bytes will be ignored.

## Command Register

The Command register is thus defined:
|Range|Purpose|
|-|-|
|`0`|Write a 1 here to clear the state of the peripheral|
|`31:1`|Reserved|

Note that the command register is a write-only register. Reading the command register will always yield the value `0`.

## Output Registers

The output registers contain the values of the output. The values contained here are valid if and only if bit 0 of the status register is `1`. 

The output is saved in chunks of unsigned 32 bit integers, where the most significant u32 is in the first register, and the least significant one is in the 16th. Note that if the peripheral is set up to output less than 512 bits, only values for all registers up to the hash length are defined, the values of the last registers are kept undefined and should not be read.
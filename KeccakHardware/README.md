# Keccak Hardware

This folder is related to the hardware part of the Project. Inside the Peripheral folder is the verilog code for the whole peripheral, and inside the Simulation folder there's a testbench for it.

To run the testbench, just `cd` to the Simulation folder and then run `make`. You should have [Icarus Verilog](https://github.com/steveicarus/iverilog) installed on your system, as a dependency.

Under the Vitis folder, there is some test code for the peripheral, alongside a quick and dirty benchmark program.

## KetchupHardwarePlatformFivePeripherals Specification

This is an hardware platform with five different Ketchup Peripherals implemented, with varying hash sizes. The peripherals and their starting addresses are thus defined:
|Index|HashSize|BaseAddress|
|-|-|-|
|0|512|`0x43C00000`|
|1|384|`0x43C10000`|
|2|256|`0x43C20000`|
|3|224|`0x43C30000`|
|4|512|`0x43C40000`|

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

It can output SHA3 hashes of varying sizes, according to the `C_SHA3_SIZE` parameter.

## Control Register

The control registers is thus defined:
|Range|Purpose|
|-|-|
|`1:0`|Number of bits to be transmitted. Ignored if bit 2 is zero|
|`2`|Is the next write to `INPUT` the last for this stream?|
|`31:3`|Reserved|

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
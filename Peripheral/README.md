# Keccak Peripheral

This is the code for the peripheral that computes SHA3 hashes. Inside the `src` folder there's code for both  the core itself (`/src/core/`), and for the bus interface (`/src/KetchupPeripheral_v1_0_S00_AXI.v`).

## Core

The core is taken from [OpenCores](https://opencores.org/projects/sha3). Two main changes have been made to it:
- It now supports all four main lenghts of SHA3: 224, 256, 384, 512.
- It now outputs hashes that are compliant with FIPS 202; so the hases are proper SHA3, as compared  to the Keccak hashes of the original core. 

The timings for this core are identical to the original one, except that there's an additional `out_size` input signal which controls what kind of hash the core should output. This input should be steady for the whole duration of the hashing process, otherwise the peripheral output is undefined. Valid values for `out_size` are:
- `00`: The output hash is SHA3-512;
- `01`: The output hash is SHA3-384;
- `10`: The output hash is SHA3-256;
- `11`: The output hash is SHA3-224.

## Testbench

Running the testbench requires a modern version of python, [Icarus Verilog](https://github.com/steveicarus/iverilog) and probably some way to visualize the resulting waveforms (Like [gtkwave](https://gtkwave.sourceforge.net/)). All the tests can be run by simply running the `make` command, and it will test all possible combinations of inputs. If you want to add more test cases, you can edit the `test_strings` array inside `maketests.py`.

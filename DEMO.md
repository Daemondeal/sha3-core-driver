# Demo

This file details the step to replicate the demo of the project.

## Setup

This setup assumes that you have the PetaLinux Tools installed at version 2023.2. These tools can be downloaded at [this link](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html).

To build the project, you have to go inside the `Petalinux/ketchup_driver` folder, and run the command `petalinux-build`. Once that's done, you either boot via JTAG using `petalinux-boot --kernel --jtag`, or via SD card using the steps detailed in the [tutorial](../Tutorials/04_petalinux_workflow.md). Don't forget to put the boot jumper in the correct position for this process.

## Steps to replicate the demo

1. Boot from SD
2. Log in as "ketchup". No password should be required.
3. Run "sha3sum 512 'hello'". The resulting hash should be "75d527c368f2efe848ecf6b073a36767800805e9eef2b1857d5f984f036eb6df891d75f72d9b154518c1cd58835286d1da9a38deba3de98b5a53e5ed78a84976".
4. Now `cd nist_test_suite`.
5. Run `./run_tests.sh`. All tests should pass.
6. Run `multiprocessing-demo blocking`. After a while, all processes should succeed, and they should all print at the end `digest[0] = 32`.
7. Run `multiprocessing-demo nonblocking`. After a while, exactly 4 processes should succeed. The ones that don't should print `Error: Resource temporarily unavailable (errno = 11)`.
8. Run `sysfs-demo &`. Press enter a couple times.
9. Run `cat /sys/class/keccak_accelerators/ketchup_driver/current_usage`. Only three peripherals should be in use.
10. Run `multiprocessing-demo nonblocking`. Now only one process should finish successfully.
11. Run `cat /sys/class/keccak_accelerators/ketchup_driver/hash_size`. There should be all possible sizes, the order doesn't matter.
12. Now run `kill -USR1 [PID]`, where `[PID]` is the PID that the sysfs-demo told you before.
13. If you now run `cat /sys/class/keccak_accelerators/ketchup_driver/current_usage`, no peripherals should be in use.
14. Run `cd` to go back to the home folder.
15. Run `cat /dev/random >> temp` and wait for a few seconds.
16. Run `du -h temp`, and if the file is bigger than 100M, go back to step 15.
17. Run `time sh -c "cat temp | sha3sum-openssl 512`. While testing, with a temp `122.4M` long, it took `0m23.634s` of `real` time.
18. Run `time sh -c "cat temp | sha3sum 512`. While testing, with a temp `122.4M` long, it took `0m7.836s` of `real` time. The hash should be exactly the same as the one you got in step 17.

## Other Notes

If you want to use sudo, you have to logout, and then log back in as `petalinux`. You will be prompted to change the password immediately.

To show the memory map, run `sudo cat /proc/iomem`

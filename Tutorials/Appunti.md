Base address del componente 0x43C00000
+ 4 per ogni successivo registro che vuoi


## Appunti:

Vogliamo provarea  fare boot dalla scheda sd, in teoria devi:

- riconfigurare non ricordo se il kernel o cosa per supportare il boot dalla scheda sd
- formattare l'sd card come hai visto nel video, estrai gli step
- nella prima partizione ci devi copiare BOOT.BIN, image.ub, boot.scr
- nella seconda partizione devi estrarre rootfs.tar.gz
- colleghi il cavo usb alla board
- apri putty a 115200
- spegni la board, metti l'sd e preghi



Per passare i file
1. petalinux-boot --jtag --kernel
2. Una volta che avvia dal temrinale di petalinux petalinux-build -c ledmodule e ti builda il modulo
3. il modulo sta in build tmp sysroot com zynq generic ledomule lib modules 6.1 extra ledmodule.ko
4. Un volta che ho il ko devo fare uuencode ./ledmoule.ko ledmodule.ko > ledmodule.enc
5. Da terminale peta fai cat > tmp
6. Vai su file di serial module prendi il ledmodule.enc, glielo mandi appena finisce control d SENZA TOCCARE NIENTE
7. ORA FAI UUDECODE TMP
8. ora fai sudo insmod ledmodule.ko


## Link per device tree
- [device tree for dummies](https://elinux.org/images/f/f9/Petazzoni-device-tree-dummies_0.pdf)
- [petalinux documentation](https://docs.xilinx.com/r/2022.2-English/ug1144-petalinux-tools-reference-guide/Configuring-Device-Tree)
- [Device tree usage](https://elinux.org/Device_Tree_Usage)

## Link vari

- [Programming with fpga manager](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841645/Solution+Zynq+PL+Programming+With+FPGA+Manager)
- [FPGA manager](https://www.kernel.org/doc/html/v4.19/driver-api/fpga/fpga-mgr.html)

drwxr-xr-x    2 root     root             0 Jan  1  1970 bdi -> 
drwxr-xr-x    2 root     root             0 Jan  1  1970 block
drwxr-xr-x    2 root     root             0 Jan  1  1970 devcoredump
drwxr-xr-x    2 root     root             0 Jan  1  1970 devlink
drwxr-xr-x    2 root     root             0 Jan  1  1970 dma
drwxr-xr-x    2 root     root             0 Jan  1  1970 drm
drwxr-xr-x    2 root     root             0 Jan  1  1970 dvb
drwxr-xr-x    2 root     root             0 Jan  1  1970 extcon
drwxr-xr-x    2 root     root             0 Jan  1  1970 fpga_bridge -> no
drwxr-xr-x    2 root     root             0 Jan  1  1970 fpga_manager -> no
drwxr-xr-x    2 root     root             0 Jan  1  1970 fpga_region -> no
drwxr-xr-x    2 root     root             0 Jan  1  1970 gpio -> nope
drwxr-xr-x    2 root     root             0 Jan  1  1970 hwmon
drwxr-xr-x    2 root     root             0 Jan  1  1970 i2c-adapter -> non usiamo i2c
drwxr-xr-x    2 root     root             0 Jan  1  1970 i2c-dev -> non usiamo i2c
drwxr-xr-x    2 root     root             0 Jan  1  1970 input -> non siamo una tastiera
drwxr-xr-x    2 root     root             0 Jan  1  1970 leds -> niente led
drwxr-xr-x    2 root     root             0 Jan  1  1970 mdio_bus
drwxr-xr-x    2 root     root             0 Jan  1  1970 mem
drwxr-xr-x    2 root     root             0 Jan  1  1970 misc
drwxr-xr-x    2 root     root             0 Jan  1  1970 mmc_host
drwxr-xr-x    2 root     root             0 Jan  1  1970 mtd
drwxr-xr-x    2 root     root             0 Jan  1  1970 net
drwxr-xr-x    2 root     root             0 Jan  1  1970 pci_bus -> non siamo attaccatti con pci
drwxr-xr-x    2 root     root             0 Jan  1  1970 power_supply -> nop
drwxr-xr-x    2 root     root             0 Jan  1  1970 pps
drwxr-xr-x    2 root     root             0 Jan  1  1970 ptp
drwxr-xr-x    2 root     root             0 Jan  1  1970 regulator
drwxr-xr-x    2 root     root             0 Jan  1  1970 remoteproc
drwxr-xr-x    2 root     root             0 Jan  1  1970 rtc
drwxr-xr-x    2 root     root             0 Jan  1  1970 scsi_device -> ha a che fare coi dischi
drwxr-xr-x    2 root     root             0 Jan  1  1970 scsi_disk -> ha a che fare coi dischi
drwxr-xr-x    2 root     root             0 Jan  1  1970 scsi_generic -> ha a che fare coi dischi
drwxr-xr-x    2 root     root             0 Jan  1  1970 scsi_host -> ha a che fare coi dischi
drwxr-xr-x    2 root     root             0 Jan  1  1970 sound -> chiaramente no
drwxr-xr-x    2 root     root             0 Jan  1  1970 spi_master -> non siamo connessi con spi
drwxr-xr-x    2 root     root             0 Jan  1  1970 thermal -> no clue ma no
drwxr-xr-x    2 root     root             0 Jan  1  1970 tty -> non lo usiamo
drwxr-xr-x    2 root     root             0 Jan  1  1970 udc
drwxr-xr-x    2 root     root             0 Jan  1  1970 uio
drwxr-xr-x    2 root     root             0 Jan  1  1970 usb_role -> no usb
drwxr-xr-x    2 root     root             0 Jan  1  1970 vc
drwxr-xr-x    2 root     root             0 Jan  1  1970 video4linux -> non usiamo video
drwxr-xr-x    2 root     root             0 Jan  1  1970 vtconsole -> ha a che fare con console
drwxr-xr-x    2 root     root             0 Jan  1  1970 wakeup -> risveglio da standby
drwxr-xr-x    2 root     root             0 Jan  1  1970 watchdog -> qualcosa con monitoraggio hw

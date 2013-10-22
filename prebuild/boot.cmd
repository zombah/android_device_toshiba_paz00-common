echo === Toshiba AC100 Bootmenu ===
setenv bootmenu_0 "Boot LNX  =mmc dev 0; ext2load mmc 0:2 0x1000000 /boot/zImage; ext2load mmc 0:2 0x2200000 /boot/initrd.gz; bootz 0x1000000 0x2200000;"
setenv bootmenu_1 "Boot SOS  =mmc dev 0; ext2load mmc 0:1 0x1000000 /boot/zImage; ext2load mmc 0:1 0x2200000 /boot/initrd-recovery.gz; bootz 0x1000000 0x2200000;"
setenv bootmenu_2 "Boot from SD =setenv bootargs console=tty0; mmc dev 1; ext2load mmc 1 0x1000000 /boot/zImage; ext2load mmc 1 0x2200000 /boot/initrd.gz; bootz 0x1000000 0x2200000;"
setenv bootmenu_3 "Boot from USB=setenv bootargs $cmdline; usb dev 0; ext2load usb 0 0x1000000 /boot/zImage; ext2load usb 0 0x2200000 /boot/initrd.gz; bootz 0x1000000 0x2200000 0x2000000;"
bootmenu 20

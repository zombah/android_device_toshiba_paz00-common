echo === Toshiba AC100 Bootmenu ===
setenv bootmenu_0 "Boot LNX CM-11.0 =ext2load mmc 0:2 0x1000000 /boot/zImage-boot-cm-11-0; ext2load mmc 0:2 0x2200000 /boot/initrd-boot-cm-11-0.gz; bootz 0x1000000 0x2200000;"
setenv bootmenu_1 "Boot SOS CM-11.0 =ext2load mmc 0:1 0x1000000 /boot/zImage-recovery-cm-11-0; ext2load mmc 0:1 0x2200000 /boot/initrd-recovery-cm-11-0.gz; bootz 0x1000000 0x2200000;"
setenv bootmenu_2 "Boot from SD fat=fatload mmc 1:1 0x1000000 /boot/zImage; fatload mmc 1:1 0x2200000 /boot/initrd.gz; bootz 0x1000000 0x2200000;"
setenv bootmenu_3 "Boot from USB ext2=ext2load usb 0:1 0x1000000 /boot/zImage; ext2load usb 0:1 0x2200000 /boot/initrd.gz; bootz 0x1000000 0x2200000 0x2000000;"
bootmenu 5

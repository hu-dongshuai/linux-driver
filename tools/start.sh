#!/bin/bash

if [ a$1 != "atest" ];
then
echo start 
qemu-system-arm -cpu cortex-a9 \
    -machine type=vexpress-a9 \
	-m 512m -kernel zImage \
	-dtb vexpress-v2p-ca9.dtb \
	-drive if=sd,driver=file,filename=a9rootfs.ext3 \
	-append "root=/dev/mmcblk0 kgdboc=tty,115200 nokaslr" \
	-fsdev local,security_model=passthrough,id=fsdev0,path=/home/h/linux/shared \
	-device virtio-9p-device,id=fs0,fsdev=fsdev0,mount_tag=host \
    -hda sdcard.ext3 \
	-serial tcp::1234,server,nowait \
	-monitor stdio\

else
qemu-system-arm -cpu cortex-a9 \
    -machine type=vexpress-a9  -serial mon:stdio \
	-m 512m -kernel zImage \
	-dtb vexpress-v2p-ca9.dtb \
	-drive if=sd,driver=file,filename=a9rootfs.ext3 \
	-append "console=ttyAMA0 console=tty1 root=/dev/mmcblk0 kgdboc=ttyAMA0" \
	-fsdev local,security_model=passthrough,id=fsdev0,path=/home/h/linux/shared \
	-device virtio-9p-device,id=fs0,fsdev=fsdev0,mount_tag=host 

fi
#-serial stdio 
#-serial tcp::1234,server,nowait \
#--serial mon:stdio \
#-drive if=scsi,driver=file,filename=a9rootfs.ext3 \
#-sd  a9rootfs.ext3 \
#-serial tcp::1234,server,nowait 
#-full-screen
#console=ttyAMA0,

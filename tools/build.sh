#!/bin/sh
echo $1
if [ "a$1" = "am" ];
then	
    make CROSS_COMPILE=arm-linux-gnueabi- ARCH=arm menuconfig
else
    make CROSS_COMPILE=arm-linux-gnueabi- ARCH=arm -j4
    cp arch/arm/boot/zImage ../../arm-pack/
	cp arch/arm/boot/dts/imx6q-sabrelite.dtb ../../arm-pack/
#    cp arch/arm/boot/dts/vexpress-v2p-ca9.dtb ../../arm-pack/
fi


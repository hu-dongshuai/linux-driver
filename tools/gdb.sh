#!/bin/sh
arm-linux-gnueabi-gdb \
-ex 'file vmlinux' \
-ex 'target remote localhost:1234' \
-ex 'add-symbol-file ../../driver/block/test.ko -s .data 0x7f002190 -s .bss 0x7f003000'

#-ex 'add-auto-load-safe-path /home/h/linux/kernel/scripts/gdb/vmlinux-gdb.py' \

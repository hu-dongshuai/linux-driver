
CURDIR := $(shell pwd)
KVERS = $(shell uname -r)

KERNEL_DIR := /lib/modules/$(KVERS)/build

KERNEL_DIR_X86_517 := /home/h/linux/kernel/5.17
KERNEL_DIR_ARM := /home/h/linux/kernel/4.14

ARCH=arm
CROSS_COMPILE=arm-linux-gnueabi-

obj-m = virnet.o

#obj-m := modulename.o(多个文件需要编译)
#modulename-objs := file1.o file2.o

#Specify flags for the module compliation.
#EXTRA_CFLAGS=-g -O0
arm:arm_modules cp_modules
x86:x86_modules
x86_517:x86_517_modules cp_modules

arm_modules:
		make -C $(KERNEL_DIR_ARM) M=$(CURDIR) modules CROSS_COMPILE=arm-linux-gnueabi- ARCH=arm CFLAGS_MODULE+=-DT_ARM
		@echo make modules for arm

x86_modules:
		make -C $(KERNEL_DIR) M=$(CURDIR) modules CFLAGS_MODULE+=-DT_X86
		@echo make modules for x86

x86_517_modules:
		make -C $(KERNEL_DIR_X86_517) M=$(CURDIR) modules CFLAGS_MODULE+=-DT_X86_517
		@echo make modules for x86_517

cp_modules:
		$(shell cp *.ko /home/h/linux/shared)

clean:
		make -C $(KERNEL_DIR) M=$(CURDIR) clean
		$(shell rm /home/h/linux/shared/*.ko)


CURDIR := $(shell pwd)
KVERS = $(shell uname -r)

#KERNEL_DIR := /lib/modules/$(KVERS)/build
KERNEL_DIR := /home/hds/myfile/fire_linux/kernel_4.19

ARCH=arm
CROSS_COMPILE=arm-linux-
export ARCH CROSS_COMPILE

#src := $(wildcard *.c)
#obj-m := $(patsubst %.c,%.o,$(src))
obj-m := platform_rgb.o

#obj-m := modulename.o(多个文件需要编译)
#modulename-objs := file1.o file2.o

#Specify flags for the module compliation.
#EXTRA_CFLAGS=-g -O0

build:kernel_modules
kernel_modules:
		make -C $(KERNEL_DIR) M=$(CURDIR) modules 
clean:
		make -C $(KERNEL_DIR) M=$(CURDIR) clean

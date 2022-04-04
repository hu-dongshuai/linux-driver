KERNEL_DIR := /home/hds/myfile/computer_science/linux/linux
CURDIR := $(shell pwd)

#Kernel modules
obj-m += global.o

#obj-m := modulename.o(多个文件需要编译)
#modulename-objs := file1.o file2.o

#Specify flags for the module compliation.
#EXTRA_CFLAGS=-g -O0

build:kernel_modules
kernel_modules:
		make -C $(KERNEL_DIR) M=$(CURDIR) modules
clean:
		make -C $(KERNEL_DIR) M=$(CURDIR) clean

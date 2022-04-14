
CURDIR := $(shell pwd)
KVERS = $(shell uname -r)

#KERNEL_DIR := /lib/modules/$(KVERS)/build
KERNEL_DIR := /home/h/linux/kernel

#Kernel modules
ifeq ($(env),global)
	obj-m = global.o
else ifeq ($(env),global_test)
	obj-m = global_test.o
else
	obj-m = test.o
endif

#obj-m := modulename.o(多个文件需要编译)
#modulename-objs := file1.o file2.o

#Specify flags for the module compliation.
#EXTRA_CFLAGS=-g -O0

build:kernel_modules cp_modules
kernel_modules:
		make -C $(KERNEL_DIR) M=$(CURDIR) modules
cp_modules:
		$(shell cp *.ko /home/h/linux/shared)
clean:
		make -C $(KERNEL_DIR) M=$(CURDIR) clean
		$(shell rm /home/h/linux/shared/test.ko)

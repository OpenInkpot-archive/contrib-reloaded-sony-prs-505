
# use the kernel build system
KERNEL_SOURCE := ./linux/ 
CROSS_COMPILE=/opt/host/armv4l/bin/armv4l-unknown-linux-

kernel:
	make -C $(KERNEL_SOURCE) M=`pwd` modules

clean:
	rm *.o

ARCH=arm
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Ilinux/include
reloaded.o: reboot.o main.o arm-mmu.o proc-arm920.o proc-macros.o
	$(CROSS_COMPILE)ld -m armelf_linux -r -o reloaded.o reboot.o main.o arm-mmu.o proc-arm920.o proc-macros.o

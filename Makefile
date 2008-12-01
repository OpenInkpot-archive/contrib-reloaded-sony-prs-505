ifeq ($(IN_KBUILD), y)
obj-m += reloaded.o

include $(TOPDIR)/Rules.make

reloaded.o: reboot.o main.o arm-mmu.o proc-arm920.o proc-macros.o
	$(CROSS_COMPILE)ld  -mno-fpu -m armelf_linux -r -o reloaded.o reboot.o main.o arm-mmu.o proc-arm920.o proc-macros.o

else

CROSS_COMPILE = arm-softfloat-linux-gnu-
KERNELSRC = /home/jek/work/src/linux/sony/EBOOK_1_2_0_P4.2_20070426_Linux_src/linux/mvl21/

all:
	$(MAKE) -C $(KERNELSRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm SUBDIRS=$(PWD) IN_KBUILD=y modules
clean:
	rm -f *.o *.flags

endif

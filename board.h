#ifndef BOARD_H
#define BOARD_H

#include <asm/arch/memory.h>

#define RAM_PHYS_START 0x08000000
/* RAM SIZE = 64M */
#define RAM_SIZE (64 * 0x100000)

/* reserve low 8MB */
#define MIN_ADDR (RAM_PHYS_START + 0x800000)

#define DEFAULT_KERNEL_PATH 	"/mnt/zImage"
#define DEFAULT_INITRD_PATH 	""
#define DEFAULT_CMDLINE		"console=ttySMX1,115200n8 ip=none"
#define DEFAULT_MACHTYPE	2007

#endif

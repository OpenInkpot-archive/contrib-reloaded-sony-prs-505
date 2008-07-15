//
// reloaded - kernel module with kexec-like functionality
// 
// original DNS-323 code - tp@fonz.de, 2007
// Cybook Gen 3 modifications - ondra.herman@gmail.com, 2008
//
#define MODULE
#define LINUX
#define __KERNEL__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/stat.h>

#include <asm/setup.h>
#include <asm/uaccess.h>
#include <asm/proc-armv/cache.h>

extern void setup_mm_for_reboot(char mode);

/* module parameters */
static char *kernel = "zImage";
MODULE_PARM(kernel, "s");
MODULE_PARM_DESC(kernel, "Kernel image file");

static char *initrd = "";
MODULE_PARM(initrd, "s");
MODULE_PARM_DESC(initrd, "initrd file");

static char *cmdline = "root=/dev/ram console=ttySAC0 loglevel=8";
MODULE_PARM(cmdline, "s");
MODULE_PARM_DESC(cmdline, "Kernel command line");

static int machtype = 193;
MODULE_PARM(machtype, "i");
MODULE_PARM_DESC(machtype, "Machine type (see linux/arch/arm/tools/mach-types)");


/* reboot.S */
/* current array sizes:
   200 for kernel
   280 for initrd

   mean segment size is 32k,
   two longs needed per segment

   kernel < 200/2 * 32k = 3.2M
   initrd < 280/2 * 32k = 4.3M
*/
   
extern unsigned long reloaded_kernel_segments[];
extern unsigned long reloaded_initrd_segments[];
extern unsigned long reloaded_taglist;
extern unsigned long reloaded_machtype;

extern void reloaded_reboot(void);
extern unsigned char reloaded_do_reboot[];
extern unsigned long reloaded_reboot_size;
extern unsigned long reloaded_reboot_start;


/* reserve low 16MB */
#define MIN_ADDR 0x01000000lu

/* */
#define SEGMENT_SIZE (32*PAGE_SIZE)
#define MIN_SEGMENT_SIZE PAGE_SIZE
static unsigned int segment_size = SEGMENT_SIZE;

static long get_size(char *filename, unsigned long *size)
{
	struct nameidata nd;
	int error;

	if(error = user_path_walk(filename, &nd))
		return error;

	*size = nd.dentry->d_inode->i_size;
	path_release(&nd);

	return 0;
}


static void load_file(char *filename, int count, unsigned long *segments)
{
        mm_segment_t old_fs = get_fs();
        int fd, i;

        void *ptr;
        unsigned long c = 0;

        int segc = 0;

        set_fs(KERNEL_DS);
        if ((fd = sys_open(filename, O_RDONLY, 0)) < 0) 
                panic("%s: open failed (%d)\n", filename, fd);
        while (c < count) {

        retry:
                while (!(ptr = kmalloc(segment_size, GFP_KERNEL))) {
                        if (segment_size == MIN_SEGMENT_SIZE)
                                panic("load_file: Out of memory\n");
                        segment_size >>= 1;
                }
                if (virt_to_phys(ptr) < MIN_ADDR) {
                        printk("not using %d bytes reserved memory at %p / %p\n", segment_size, ptr, __virt_to_phys(ptr));
                        goto retry;
                }

                i = sys_read(fd, ptr, segment_size);
                printk("loaded %d of %d bytes at %p / %p\n", i, count, ptr, __virt_to_phys(ptr));
                if (i > 0) {
                        *segments++ = virt_to_phys(ptr);
                        *segments++ = (unsigned long)i;
                        c += i;
                        segc++;
                }
                if (i < 0)
                        panic("load_file: read error\n");
        }
        *segments = 0;
        sys_close(fd);
        printk("%s: OK (%d segments)\n", filename, segc);
        set_fs(old_fs);
}


static void *make_taglist(int initrd_size) 
{
        void *taglist;
        struct tag *t;
        int i;
        unsigned long mem_start = 0x30000000;
        unsigned long mem_size = 0x01000000; //(unsigned long)__virt_to_phys(high_memory);

        if (!(t = taglist = kmalloc(MIN_SEGMENT_SIZE, GFP_KERNEL)))
                panic("make_taglist: Out of memory\n");

        t->hdr.tag = ATAG_CORE;
        t->hdr.size = tag_size(tag_core);
        t->u.core.flags = 1;
        t->u.core.pagesize = PAGE_SIZE;
        t->u.core.rootdev = 0x0; //0x00010000lu;

        printk("CMDLINE: %s\n", cmdline);
        t = tag_next(t);
        i = strlen(cmdline)+1;
        t->hdr.size = (sizeof(struct tag_header) + i+3) >> 2;
        t->hdr.tag = ATAG_CMDLINE;
        strcpy(t->u.cmdline.cmdline, cmdline);

        printk("MEM: start %08lx size %luMB\n", mem_start, mem_size >> 20);
        t = tag_next(t);
        t->hdr.size = tag_size(tag_mem32);
        t->hdr.tag = ATAG_MEM;
        t->u.mem.size = mem_size;
        t->u.mem.start = mem_start;

        if (initrd_size > 0) {
                printk("INITRD: start %08lx size %d\n", 0x800000lu, initrd_size);
                t = tag_next(t);
                t->hdr.size = tag_size(tag_initrd);
                t->hdr.tag = ATAG_INITRD2;
                t->u.initrd.start = 0x800000;
                t->u.initrd.size = initrd_size;
        }

        t = tag_next(t);
        t->hdr.size = 0;
        t->hdr.tag = ATAG_NONE;

        return taglist;
}


/* main function */
int init_module()
//static int __init init_module(void)
{
        mm_segment_t old_fs;
        int err = 0;
        unsigned long kernel_size;
        unsigned long initrd_size;
        void *reloaded_reboot_code;

        printk("reloaded for Cybook Gen 3, 2007 tp@fonz.de, 2008 ondra.herman@gmail.com\n");

        /* load kernel and initrd */
        old_fs = get_fs();
        set_fs(KERNEL_DS);
        if ((err = get_size(kernel, &kernel_size)) < 0) 
                printk("%s: stat failed (%d)\n", kernel, err);
        else if (*initrd && (err = get_size(initrd, &initrd_size)) < 0)
                printk("%s: stat failed (%d)\n", initrd, err);
        set_fs(old_fs);
        if (err < 0)
                return -ENOENT;

        printk("%s: %lld bytes\n", kernel, kernel_size);
        load_file(kernel, kernel_size, reloaded_kernel_segments);
        if (*initrd) {
                printk("%s: %lld bytes\n", initrd, initrd_size);
                load_file(initrd, initrd_size, reloaded_initrd_segments);
        }

        /* */
        reloaded_machtype = machtype;
        printk("reloaded_machtype = %ld\n", reloaded_machtype);
        reloaded_taglist = virt_to_phys(make_taglist(*initrd ? initrd_size : 0));
        printk("reloaded_taglist  = %lx (%p)\n", reloaded_taglist, phys_to_virt(reloaded_taglist));
        if (!(reloaded_reboot_code = kmalloc(reloaded_reboot_size, GFP_KERNEL))) 
                panic("reboot code: Out of memory\n");
        printk("copying %lu bytes reboot code from %p to %p\n",
               reloaded_reboot_size, reloaded_do_reboot, reloaded_reboot_code);
        memcpy(reloaded_reboot_code, reloaded_do_reboot, reloaded_reboot_size);
        reloaded_reboot_start = virt_to_phys(reloaded_reboot_code);
        printk("reloaded_reboot_start  = %lx\n", reloaded_reboot_start);

        /* go */
        printk(KERN_INFO "Reloading...\n");
        flush_icache_range((unsigned long)reloaded_reboot_code, (unsigned long)reloaded_reboot_code + reloaded_reboot_size);
        cpu_arm920_proc_fin();
        setup_mm_for_reboot(0);
        reloaded_reboot();

        panic("FAILED\n");
        return -1;
}

MODULE_LICENSE("GPL");

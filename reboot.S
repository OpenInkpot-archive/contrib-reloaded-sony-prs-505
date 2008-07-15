
        .globl  reloaded_reboot
reloaded_reboot:
        ldr     r0, reloaded_reboot_start
        mov     pc, r0

        .globl  reloaded_reboot_start
reloaded_reboot_start:       
        .long   0x0

                
        .globl  reloaded_do_reboot
reloaded_do_reboot:

        mov     r0, #0
        mov     r1, #1
        mov     r2, #2
        mov     r3, #3
        mov     r4, #4
        mov     r5, #5
        mov     r6, #6
        mov     r7, #7
        mov     r8, #8
        mov     r9, #9

        /* copied from cpu_arm926_reset() */
      	mov     ip, #0
        mcr     p15, 0, ip, c7, c7, 0		@ invalidate I,D caches
        mcr     p15, 0, ip, c7, c10, 4		@ drain WB
        mcr     p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
        mrc     p15, 0, ip, c1, c0, 0		@ ctrl register
        bic     ip, ip, #0x000f             @ ............wcam
        bic     ip, ip, #0x1100			    @ ...i...s........
        mcr     p15, 0, ip, c1, c0, 0		@ ctrl register


        /* inifinite loop */
@        mov     lr, pc
@        mov     r0, r0
@        mov     pc, lr

        
        /* copy tag list */
        ldr     r1, reloaded_taglist
        mov     r2, #0x100
next_tag:      
        ldr     r0, [r1]
        cmp     r0, #0
        beq     tags_done
copy_tag:
        ldmia   r1!, {r3}
        stmia   r2!, {r3}
        subs    r0, r0, #1
        beq     next_tag
        b       copy_tag
tags_done:
        stmia   r2!, {r0}
        stmia   r2!, {r0}

        
        /* inifinite loop */
@        mov     lr, pc
@        mov     r0, r0
@        mov     pc, lr

        /* copy initrd */
        mov     r2, #0x800000
        adr     r3, reloaded_initrd_segments
copy_initrd_next_segment:
        ldmia   r3!, {r1}          @ r1 = ptr
        cmp     r1, #0x0
        beq     copy_initrd_done
        ldmia   r3!, {r0}          @ r0 = count
copy_initrd:
        ldmia   r1!, {r4}
        stmia   r2!, {r4}
        subs    r0, r0, #4
        bgt     copy_initrd
        b       copy_initrd_next_segment
copy_initrd_done:               


        /* inifinite loop */
@        mov     lr, pc
@        mov     r0, r0
@        mov     pc, lr
        
                                
        /* copy kernel */
        mov     r2, #0x30000000
	add	r2, r2, #0x8000
        adr     r3, reloaded_kernel_segments
copy_kernel_next_segment:
        ldmia   r3!, {r1}          @ r1 = ptr
        cmp     r1, #0x0
        beq     copy_kernel_done
        ldmia   r3!, {r0}          @ r0 = count
copy_kernel:
        ldmia   r1!, {r4}
        stmia   r2!, {r4}
        subs    r0, r0, #4
        bgt     copy_kernel
        b       copy_kernel_next_segment
copy_kernel_done:               

        /* inifinite loop */
@        mov     lr, pc
@        mov     r0, r0
@        mov     pc, lr

        
        /* setup registers and jump */                
        mov     r0, #0
        ldr     r1, reloaded_machtype
        mov     r2, #0x100
        mov     lr, #0x30000000
	add	lr, lr, #0x8000
        mov     pc, lr


        .globl  reloaded_machtype
reloaded_machtype:        
        .long   0x0

        
        .globl  reloaded_taglist
reloaded_taglist:
        .long   0x0


        .globl  reloaded_kernel_segments
reloaded_kernel_segments:
        .rept   200
        .long   0x0
        .endr
        
              
        .globl  reloaded_initrd_segments
reloaded_initrd_segments:
        .rept   280
        .long   0x0
        .endr
        
                                        
        .globl  reloaded_reboot_size
reloaded_reboot_size:
        .long   reloaded_reboot_end - reloaded_do_reboot

reloaded_reboot_end:



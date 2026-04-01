.section ".text.boot"
.global _start

_start:
    // only core 0 runs, park the rest
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbnz    x1, .park

    // set up stack (grows downward from _start)
    ldr     x1, =_start
    mov     sp, x1

    // zero out BSS
    ldr     x1, =__bss_start
    ldr     x2, =__bss_end
.bss_loop:
    cmp     x1, x2
    b.ge    .bss_done
    str     xzr, [x1], #8
    b       .bss_loop
.bss_done:

    // jump to C kernel
    bl      kernel_main

    // if kernel_main ever returns, hang
.park:
    wfe
    b       .park

.section ".bss.boot"
// ==========================================
//  @file     boot.S
//  @brief    aarch64-bit code for booting the kernel
//            also responsible for setting up the c/c++ env
// ==========================================

.section ".text.boot"
.global _entry

// ==========================================
// @function            start
// @brief               start of the porgram
// ==========================================
_entry:
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbnz    x1, .halt
    ldr     x1, =KSTACK_TOP
    mov     sp, x1
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    sub     x1, x1, x0
    bl      __memzero
    bl      kernel_entry
    .halt:
        wfe
        b       .halt

// ==========================================
// @function            memzero
// @brief               zeros a memory region
// @param x0            start addr
// @param x1            total size
// ==========================================
__memzero:
    cbz     x1, .done
    .loop:
        str     xzr, [x0], #8
        subs    x1, x1, #8
        b.gt    .loop
    .done:
        ret

.section ".bss.boot"

// 2 MB of kernel stack
.align 4 
KSTACK_BOTTOM:
    .skip 1024 * 1024 * 2
KSTACK_TOP:
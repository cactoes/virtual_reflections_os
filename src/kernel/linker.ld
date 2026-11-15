/*==========================================
 * @file     linker.ld
 * @brief    kernel linker script
 *
 * the boot code is loaded and runs at its physical address (1mb).
 * the kernel is linked in the higher half, but loaded at its
 * physical address (virtual address - KERNEL_VIRTUAL_BASE).
 *==========================================*/

ENTRY(_boot_amd64)

/* memory layout */
BOOT_PHYSICAL_BASE      = 0x100000;
LARGE_PAGE_SIZE         = 0x200000;

/* limit for the kernel (1gb max) */
BOOT_IDENTITY_LIMIT     = 0x40000000;

SECTIONS {
    . = BOOT_PHYSICAL_BASE;

    /*==========================================
     * boot (physical, identity mapped)
     *==========================================*/
    .boot.multiboot : ALIGN(8) {
        KEEP(*(.boot.multiboot))
    }

    .boot : ALIGN(4K) {
        *(.boot.text)
        *(.boot.rodata)
        *(.boot.data)
    }

    .boot.bss (NOLOAD) : ALIGN(4K) {
        *(.boot.bss)
    }

    /* kernel (higher half) */
    . = KERNEL_VIRTUAL_BASE + ALIGN(., LARGE_PAGE_SIZE);

    .text : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE) ALIGN(4K) {
        __lnk_text_start = .;
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        __lnk_text_end = .;
    }

    .data : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE) ALIGN(4K) {
        __lnk_data_start = .;
        *(.data)
        *(.data.*)
        __lnk_data_end = .;
    }

    .bss (NOLOAD) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE) ALIGN(4K) {
        __lnk_bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __lnk_bss_end = .;
    }

    /* exports */
    __lnk_bss_size              = __lnk_bss_end - __lnk_bss_start;
    __lnk_kernel_end_virtual    = .;
    __lnk_kernel_end_physical   = __lnk_kernel_end_virtual - KERNEL_VIRTUAL_BASE;

    /* checks */
    ASSERT(__lnk_kernel_end_physical <= BOOT_IDENTITY_LIMIT, "kernel too big (1gb max)")

    /DISCARD/ : {
        *(.fini_array*)
        *(.comment)
        *(.eh_frame)
    }
}

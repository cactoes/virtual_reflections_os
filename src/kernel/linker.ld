/* ========================================== */
/* @file    linker.ld                         */
/* @brief   linker script for the kernel      */
/* ========================================== */

ENTRY(entry)

SECTIONS {
    . = 2M;

    .boot : ALIGN(4) {
        KEEP(*(.multiboot))
    }

    .text : ALIGN(4K) {
        *(.text*)
        *(.rodata)
    }

    .data : ALIGN(4K) {
        __lnk_start_ctors = .;
        KEEP(*( .init_array ));
        KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
        __lnk_end_ctors = .;
        *(.data*)
    }

    .bss : ALIGN(4K) {
        __lnk_bss_start = ABSOLUTE(.);
        *(.bss)
        *(COMMON)
    }
    __lnk_bss_size = ABSOLUTE(.) - __lnk_bss_start;

    __lnk_end_kernel = .;

    /DISCARD/ : {
        *(.fini_array*)
        *(.comment)
    }
}
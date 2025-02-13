/* ========================================== */
/* @file    linker.ld                         */
/* @brief   linker script for the kernel      */
/* ========================================== */

/* entrypoint subroutine name */
ENTRY(loader)

/* different code segments, offset at 2MB */
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
        start_ctors = .;
        KEEP(*( .init_array ));
        KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
        end_ctors = .;
        *(.data*)
    }

    .bss : ALIGN(4K) {
        bss_start = ABSOLUTE(.);
        *(.bss)
        *(COMMON)
    }
    bss_size = ABSOLUTE(.) - bss_start;

    end_kernel = .;

    /DISCARD/ : {
        *(.fini_array*)
        *(.comment)
    }
}
ENTRY(entry)

KERNEL_VIRTUAL_BASE = 0xFFFFF80000000000;

SECTIONS {
    . = 0x100000;

    .multiboot2 : ALIGN(8) {
        KEEP(*(.multiboot2))
    }

    .boot : ALIGN(4K) {
        KEEP(*(.multiboot1))
        *(.boot.text)
        *(.boot.rodata)
        *(.boot.data)
    }

    .boot.bss (NOLOAD) : ALIGN(4K) {
        *(.boot.bss)
    }

    . += KERNEL_VIRTUAL_BASE;
    . = ALIGN(0x200000);

    .text : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE) ALIGN(4K) {
        _text_start = .;
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        _text_end = .;
    }

    .data : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE) ALIGN(4K) {
        _data_start = .;
        __lnk_start_ctors = .;
        KEEP(*(.init_array))
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        __lnk_end_ctors = .;
        *(.data)
        *(.data.*)
        _data_end = .;
    }

    .bss (NOLOAD) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE) ALIGN(4K) {
        __lnk_bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __lnk_bss_end = .;
    }

    __lnk_bss_size = __lnk_bss_end - __lnk_bss_start;
    __lnk_end_kernel = .;
    __lnk_end_kernel_phys = __lnk_end_kernel - KERNEL_VIRTUAL_BASE;

    /DISCARD/ : {
        *(.fini_array*)
        *(.comment)
        *(.eh_frame)
    }
}
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_entry)

SECTIONS {
    . = 0x400000;

    .text : {
        *(.text*)
        *(.rodata*)
    }

    .data ALIGN(0x1000) : {
        *(.data*)
    }

    .bss ALIGN(0x1000) : {
        *(.bss*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}
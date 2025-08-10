OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    .text ALIGN(0x1000) : {
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
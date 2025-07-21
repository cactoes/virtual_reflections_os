OUTPUT_FORMAT(elf64-x86-64)
ENTRY(driver_init)

SECTIONS
{
    /* Text segment: code + read-only data */
    .text ALIGN(0x1000) : {
        *(.text*)           /* all code sections */
        *(.rodata*)         /* read-only data */
    }

    /* Data segment: initialized data */
    .data ALIGN(0x1000) : {
        *(.data*)
    }

    /* BSS segment: zero-initialized data */
    .bss ALIGN(0x1000) : {
        *(.bss*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}
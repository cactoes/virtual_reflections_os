ENTRY(_start)

SECTIONS {
    . = 0x80000;
 
    .text : { 
        KEEP(*(.boot)); 
        *(.text*) 
    } 
 
    .rodata ALIGN(16) : { 
        *(.rodata*) 
    } 
 
    .data ALIGN(16) : { 
        *(.data*) 
    } 

    . = ALIGN(8);
    __bss_start = .;
    .bss : { *(.bss*) *(COMMON) }
    . = ALIGN(8);
    __bss_end = .;
}
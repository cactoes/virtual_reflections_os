ENTRY(_entry)

SECTIONS {
    . = 0x40000000;
 
    .text : { 
        KEEP(*(.text.boot));
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
    .bss : {
        *(.bss.boot)
        *(.bss*)
        *(COMMON)
    }
    . = ALIGN(8);
    __bss_end = .;
}
;==========================================
; @file     entry.asm
; @brief    32-bit code for setting up 64-bit env
;           this code is mess & not setup well
;==========================================

section .multiboot
    dd 0x1BADB002                           ; magic
    dd (1 << 0 | 1 << 1)                    ; flags
    dd -(0x1BADB002 + (1 << 0 | 1 << 1))    ; checksum

section .text
    ; 64 bit functions
    extern boot_kernel

    ; globals
    global entry
    global MB_MAGIC
    global __end_bss_keep

bits 32
entry:
    ; store multiboot struct
    mov [MB_MAGIC],  eax
    mov [MB_INFO],   ebx

    ; setup boot stack
    mov esp,    BSTACK_TOP
    and esp,    -16

    ; setup paging & long mode
    call setup_page_tables
    call enable_paging

    ; setup simd structs
    mov     eax,    cr0
    or      eax,    0x2    ; monitor coprocessor bit
    and     eax,    ~0x4   ; emulation bit
    mov     cr0,    eax

    mov     eax,    cr4
    or      eax,    0x200    ; OSFXSR
    or      eax,    0x400    ; OSXMMEXCPT
    mov     cr4,    eax

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:boot_kernel

setup_page_tables:
    mov     eax,            PDPT
    or      eax,            0b11 ; present, writable
    mov     [PML4T],        eax

    mov     eax,            PDT
    or      eax,            0b11 ; present, writable
    mov     [PDPT],         eax

    mov ecx, 0 ; counter

.loop:
    mov     eax,                0x200000 ; 2MB
    mul     ecx
    or      eax,                0b10000011 ; present, writable, huge
    mov     [PDT + ecx * 8],    eax

    inc ecx ; increment counter
    cmp ecx, 512 ; check if table is mapped
    jne .loop ; if not continue

    ret

enable_paging:
    ; pass page table to cpu
    mov eax, PML4T
    mov cr3, eax

    ; enable physycal address extension
    mov     eax,        cr4
    or      eax,        1 << 5
    mov     cr4,        eax

    ; enable long mode
    mov     ecx,    0xC0000080
    ; read model specific
    rdmsr
    ; set long mode
    or      eax,    1 << 8
    ; write model specific
    wrmsr

    ; enable paging
    mov     eax,        cr0
    or      eax,        1 << 31
    mov     cr0,        eax

    ret

section .bss
align 4096
; page table for jumping to x64, discarded later
PML4T:      resb 4096
PDPT:       resb 4096   
PDT:        resb 4096

; 512 b of boot stack
BSTACK_BOTTOM: resb 512
BSTACK_TOP:

; muliboot struct
align 8
MB_MAGIC:    resq 1; uint64_t
MB_INFO:     resq 1; void*
__end_bss_keep:     resq 1; uint64_t

section .rodata
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
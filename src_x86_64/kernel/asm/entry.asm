;==========================================
; @file     entry.asm
; @brief    32-bit code for setting up 64-bit env
;           this code is mess & not setup well
;==========================================

; section .multiboot1
;     dd 0x1BADB002                           ; magic
;     dd (1 << 0 | 1 << 1)                    ; flags
;     dd -(0x1BADB002 + (1 << 0 | 1 << 1))    ; checksum

section .multiboot2
multiboot2_start:
    dd 0xE85250D6
    dd 0x0
    dd multiboot2_end - multiboot2_start
    dd -(0xE85250D6 + 0x0 + (multiboot2_end - multiboot2_start))

    dw 0
    dw 0
    dd 8
multiboot2_end:

section .boot.text
    ; 64 bit functions
    extern boot_kernel

    ; linker variables
    extern __lnk_end_kernel_phys

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
    mov     ebx,            PML4T
    mov     eax,            PDPT
    or      eax,            0b011
    mov     [ebx],          eax

    mov     ebx,            PDPT
    mov     eax,            PDT
    or      eax,            0b011
    mov     [ebx],          eax

    mov     eax,            __lnk_end_kernel_phys
    add     eax,            0x1FFFFF
    shr     eax,            21
    mov     ecx,            eax
    xor     edx,            edx
    .identity_loop:
        mov     eax,        edx
        shl     eax,        21
        or      eax,        0b10000011
        mov     ebx,        PDT
        mov     [ebx + edx * 8], eax
        inc     edx
        cmp     edx,        ecx
        jne     .identity_loop

    mov     ebx,            PML4T
    mov     eax,            PDPT_HIGH
    or      eax,            0b011
    mov     [ebx + 496 * 8], eax

    mov     ebx,            PDPT_HIGH
    mov     eax,            PDT_HIGH
    or      eax,            0b011
    mov     [ebx],          eax

    xor     edx,            edx
    .high_loop:
        mov     eax,        edx
        shl     eax,        21
        or      eax,        0b10000011
        mov     ebx,        PDT_HIGH
        mov     [ebx + edx * 8], eax
        inc     edx
        cmp     edx,        ecx
        jne     .high_loop
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

section .boot.bss
align 4096
; page table for jumping to x64, discarded later
PML4T:      resb 4096
PDPT:       resb 4096   
PDT:        resb 4096

PDPT_HIGH:  resb 4096
PDT_HIGH:   resb 4096

; 512 b of boot stack
BSTACK_BOTTOM: resb 512
BSTACK_TOP:

; muliboot struct
align 8
MB_MAGIC:    resq 1; uint64_t
MB_INFO:     resq 1; void*
__end_bss_keep:     resq 1; uint64_t

section .boot.rodata
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
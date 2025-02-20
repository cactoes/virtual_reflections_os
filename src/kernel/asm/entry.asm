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
    extern boot_kernel
    global entry
    global multiboot_magic

bits 32
entry:
    ; store multiboot struct
    mov [multiboot_magic],  eax
    mov [multiboot_info],   ebx

    ; setup boot stack
    mov esp,    boot_stack
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
    or      eax,    0x200  ; OSFXSR
    or      eax,    0x400  ; OSXMMEXCPT
    mov     cr4,    eax

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:boot_kernel

setup_page_tables:
    mov     eax,                    page_table_l3
    or      eax,                    0b11 ; present, writable
    mov     [page_table_l4],        eax

    mov     eax,                    page_table_l2
    or      eax,                    0b11 ; present, writable
    mov     [page_table_l3],        eax

    mov ecx, 0 ; counter

.loop:
    mov     eax,                            0x200000 ; 2MB
    mul     ecx
    or      eax,                            0b10000011 ; present, writable, huge
    mov     [page_table_l2 + ecx * 8],      eax

    inc ecx ; increment counter
    cmp ecx, 512 ; check if table is mapped
    jne .loop ; if not continue

    ret

enable_paging:
    ; pass page table to cpu
    mov eax, page_table_l4
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
    global __end_bss_keep
    align 4096
page_table_l4: ; PML4T
    resb 4096
page_table_l3: ; PDPT
    resb 4096   
page_table_l2: ; PDT
    resb 4096
boot_stack_bottom:
    resb 4096 * 4; 4 kb
boot_stack:
align 8
; muliboot struct
multiboot_magic: resq 1 ; uint64_t
multiboot_info:  resq 1 ; void*
; uint64_t bss end ptr
__end_bss_keep: resq 1

section .rodata
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
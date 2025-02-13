;==========================================
; @file     loader.asm
; @brief    32-bit code for setting up 64-bit env
;           this code is mess & not setup well
;==========================================

section .multiboot
    dd 0x1BADB002         ; MAGIC
    dd (1 << 0 | 1 << 1)  ; FLAGS
    dd -(0x1BADB002 + (1 << 0 | 1 << 1))  ; CHECKSUM

section .text
    extern loader64
    global loader
    global multiboot_magic

bits 32
loader:
    mov [multiboot_magic], eax   ; Save Multiboot magic
    mov [multiboot_info], ebx    ; Save Multiboot info pointer

    mov esp, boot_stack
    and esp, -16

    call setup_page_tables
    call enable_paging

    mov eax, cr0
    or eax, 0x2    ; Set MP (Monitor Coprocessor) bit
    and eax, ~0x4  ; Clear EM (Emulation) bit
    mov cr0, eax

    mov eax, cr4
    or eax, 0x200  ; Set OSFXSR (Operating System Support for FXSAVE/FXRSTOR)
    or eax, 0x400  ; Set OSXMMEXCPT (OS Support for Unmasked SIMD Exceptions)
    mov cr4, eax

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:loader64

setup_page_tables:
    mov eax, page_table_l3
    or eax, 0b11 ; present, writable
    mov [page_table_l4], eax

    mov eax, page_table_l2
    or eax, 0b11 ; present, writable
    mov [page_table_l3], eax

    ; (outdated) map ~2mb in the l1 table
    mov ecx, 0 ; counter

.loop:
    mov eax, 0x200000 ; 2MB
    mul ecx
    or eax, 0b10000011 ; present, writable, huge
    mov [page_table_l2 + ecx * 8], eax

    inc ecx ; increment counter
    cmp ecx, 512 ; check if table is mapped
    jne .loop ; if not continue

    ret

enable_paging:
    ; pass page table to cpu
    mov eax, page_table_l4
    mov cr3, eax

    ; enable physycal address extension
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; enable long mode
    mov ecx, 0xC0000080
    ; read model specific
    rdmsr
    ; set long mode
    or eax, 1 << 8
    ; write model specific
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

error:
    ; print "ERR: __CODE__"
    mov dword [0xB8000], 0x4F524F45
    mov dword [0xB8004], 0x4F3A4F52
    mov dword [0xB8008], 0x4F204F20
    mov byte [0xB800A], al

    ; "freeze" cpu
    hlt

section .bss
    global end_bss_keep
    align 4096
; https://wiki.osdev.org/Setting_Up_Long_Mode#Setting_up_the_Paging
page_table_l4: ; PML4T (page map level 4 table)
    resb 4096
page_table_l3: ; PDPT (page directory pointer table)
    resb 4096   
page_table_l2: ; PDT (page directory table)
    resb 4096
boot_stack_bottom:
    resb 4096 * 4; 1 kb pre stack
boot_stack:
align 8
; muliboot struct
multiboot_magic: resq 1 ; uint64_t
multiboot_info:  resq 1 ; void*
; uint64_t bss end ptr
end_bss_keep: resq 1

section .rodata
; https://wiki.osdev.org/Global_Descriptor_Table
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
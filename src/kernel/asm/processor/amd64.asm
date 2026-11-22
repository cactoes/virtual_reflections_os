;==========================================
; @file     amd64.asm
; @brief    32-bit code for jumping to 64-bit c
;==========================================

; multiboot2 flags
MB2_HEADER_MAGIC            equ 0xE85250D6
MB2_ARCH_I386               equ 0x0
MB2_BOOTLOADER_MAGIC        equ 0x36D76289

MB2_TAG_TYPE_FRAMEBUFFER    equ 5
MB2_TAG_FLAG_OPTIONAL       equ (1 << 0)
MB2_FRAMEBUFFER_BPP32       equ 32
MB2_FRAMEBUFFER_BPP_ANY     equ 0

; gdt flags
GDT_ACCESS_RING0            equ (0 << 0)
GDT_ACCESS_READWRITE        equ (1 << 1)
GDT_ACCESS_EXECUTABLE       equ (1 << 3)
GDT_ACCESS_SEGMENT          equ (1 << 4)
GDT_ACCESS_PRESENT          equ (1 << 7)
GDT_LONG_MODE               equ (1 << 5)
GDT_GRANULARITY             equ (1 << 7)

; cpuid flags
CPUID_PAE                   equ (1 << 6)
CPUID_LONG_MODE             equ (1 << 29)

; cpuid functions
CPUID_EXTENDED_MAX          equ 0x80000000
CPUID_EXTENDED_FEATURES     equ 0x80000001

FLAGS_ID                    equ (1 << 21)

; page table entry flags
PF_PRESENT                  equ (1 << 0)
PF_READ_WRITE               equ (1 << 1)
PF_PAGE_SIZE                equ (1 << 7)

; page table layout
PAGE_TABLE_SIZE             equ 4096
PAGE_TABLE_COUNT            equ 3
PAGE_TABLE_ENTRIES          equ 512

; CR4 flags
CR4_PAE                     equ (1 << 5)
CR4_OSFXSR                  equ (1 << 9)
CR4_OSXMMEXCPT              equ (1 << 10)

; CR0 flags
CR0_PG                      equ (1 << 31)
CR0_MP                      equ (1 << 1)
CR0_EM                      equ (1 << 2)

; msr
MSR_EFER                    equ 0xC0000080
EFER_LME                    equ (1 << 8)

; kernel mapping
KERNEL_PML4_INDEX           equ ((KERNEL_VIRTUAL_BASE >> 39) & 0x1FF)
PML4_RECURSIVE_INDEX        equ 511

bits 32
section .boot.text
    ; remote 64 bit routine
    extern amd64_entry

    ; globals
    global _boot_amd64
    global page_table_l4
    global stack_top

;==========================================
; @function     _boot_amd64
; @brief        amd64 entry point for the kernel.
;               handles setting up of 64 bit (c) environment
;==========================================
_boot_amd64:
    ; check multiboot magic
    cmp eax, MB2_BOOTLOADER_MAGIC
    jne .stop

    ; setup a stack
    mov esp,    stack_top
    and esp,    -16

    ; save multiboot structure
    push ebx ; multiboot structure

    call check_cpuid
    jz   .stop

    call check_pae
    jz   .stop

    call check_long_mode
    jz   .stop

    call init_boot_page_table
    call enable_paging

    lgdt [boot_gdtr]

    pop edi ; multiboot structure (first argument)

    ; far jump to 64 bit code
    jmp boot_64_code_entry:long_mode_jmp

.stop:
    cli
    hlt
    jmp .stop

;==========================================
; @function     check_cpuid
; @brief        check if cpuid is supported
; @remarks      ZF=1 not supported, ZF=0 supported
;==========================================
check_cpuid:
    pushfd
    pop     eax
    mov     ecx, eax
    xor     eax, FLAGS_ID
    push    eax
    popfd

    pushfd
    pop     eax
    push    ecx
    popfd

    cmp     eax, ecx
    je      .fail

    xor     eax, eax
    inc     eax
    ret

.fail:
    xor eax, eax
    ret

;==========================================
; @function     check_pae
; @brief        check if pae is supported
; @remarks      ZF=1 not supported, ZF=0 supported
;==========================================
check_pae:
    mov     eax, 1
    cpuid

    test    edx, CPUID_PAE
    jz      .fail

    xor     eax, eax
    inc     eax
    ret

.fail:
    xor     eax, eax
    ret

;==========================================
; @function     check_long_mode
; @brief        check if long mode is supported
; @remarks      ZF=1 not supported, ZF=0 supported
;==========================================
check_long_mode:
    mov     eax, CPUID_EXTENDED_MAX
    cpuid

    cmp     eax, CPUID_EXTENDED_FEATURES
    jb      .fail

    mov     eax, CPUID_EXTENDED_FEATURES
    cpuid

    test    edx, CPUID_LONG_MODE
    jz      .fail

    xor     eax, eax
    inc     eax
    ret

.fail:
    xor     eax, eax
    ret

;==========================================
; @function     init_boot_page_table
; @brief        identity maps the first 1gb using 2mb pages
;==========================================
init_boot_page_table:
    cld
    mov     edi,    page_table.l4
    xor     eax,    eax
    mov     ecx,    (PAGE_TABLE_SIZE * PAGE_TABLE_COUNT) / 4
    rep     stosd

    mov     eax,    page_table.l3
    or      eax,    PF_PRESENT | PF_READ_WRITE
    mov     [page_table.l4],                            eax
    mov     [page_table.l4 + KERNEL_PML4_INDEX * 8],    eax

    mov     eax,    page_table.l4
    or      eax,    PF_PRESENT | PF_READ_WRITE
    mov     [page_table.l4 + PML4_RECURSIVE_INDEX * 8], eax

    mov     eax,    page_table.l2
    or      eax,    PF_PRESENT | PF_READ_WRITE
    mov     [page_table.l3], eax

    xor     ecx,    ecx

.loop:
    mov     eax,                        ecx
    shl     eax,                        21
    or      eax,                        PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE
    mov     [page_table.l2 + ecx * 8],  eax
    inc     ecx
    cmp     ecx,                        PAGE_TABLE_ENTRIES
    jne     .loop

    ret

;==========================================
; @function     enable_paging
; @brief        enable memory paging
;==========================================
enable_paging:
    mov     eax, page_table.l4
    mov     cr3, eax

    mov     eax, cr4
    or      eax, CR4_PAE
    mov     cr4, eax

    mov     ecx, MSR_EFER
    rdmsr
    or      eax, EFER_LME
    wrmsr

    mov     eax, cr0
    or      eax, CR0_PG
    mov     cr0, eax

    ret

section .boot.multiboot
; define the multiboot2 structure and request a frame buffer
align 8
multiboot2:
.magic:         dd MB2_HEADER_MAGIC
.arch:          dd MB2_ARCH_I386
.length:        dd multiboot2_end - multiboot2
.checksum:      dd -(MB2_HEADER_MAGIC + MB2_ARCH_I386 + (multiboot2_end - multiboot2))

multiboot2_tag_framebuffer:
.type:          dw MB2_TAG_TYPE_FRAMEBUFFER
.flags:         dw MB2_TAG_FLAG_OPTIONAL
.size:          dd .end - multiboot2_tag_framebuffer
.width:         dd 0 ; no preference
.height:        dd 0 ; no preference
.bpp:           dd MB2_FRAMEBUFFER_BPP32
.end:

align 8
multiboot2_tag_end:
.type:          dw 0
.flags:         dw 0
.size:          dd 8
multiboot2_end:

section .boot.bss
; define the kernel stack
align 16
stack:
    resb 0x200000 ; 2mb
stack_top:

; define a basic page table for jumping to 64 bit
; we only need to go 3 deep since we are only allocating 2mb pages
; the tables must stay contiguous (cleared in one go)
align 4096
page_table:
.l4:    resb PAGE_TABLE_SIZE
.l3:    resb PAGE_TABLE_SIZE
.l2:    resb PAGE_TABLE_SIZE

page_table_l4: equ page_table.l4

section .boot.rodata
; boot gdt
align 8
zero_entry:
    dw 0 ; limit
    dw 0 ; base_low
    db 0 ; base_mid
    db 0 ; access
    db 0 ; granularity
    db 0 ; base_high

boot_64_code_entry: equ $ - zero_entry
    dw 0
    dw 0
    db 0
    db GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READWRITE
    db GDT_LONG_MODE | GDT_GRANULARITY
    db 0

boot_gdtr:
.limit:             dw $ - zero_entry - 1
.address:           dd zero_entry

section .boot.text
bits 64
long_mode_jmp:
    ; upper half of rdi is undefined after the mode switch
    mov     edi, edi

    ; drop the stale 32 bit segment selectors
    xor     eax, eax
    mov     ss,  ax
    mov     ds,  ax
    mov     es,  ax
    mov     fs,  ax
    mov     gs,  ax

    ; move stack and multiboot pointer into the higher half
    mov     rcx, KERNEL_VIRTUAL_BASE
    mov     rsp, stack_top
    and     rsp, -16
    add     rsp, rcx
    add     rdi, rcx

    ; terminate stack walks
    xor     ebp, ebp

    mov     rax, amd64_entry
    call    rax

.stop:
    cli
    hlt
    jmp .stop
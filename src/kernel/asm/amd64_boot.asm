;==========================================
; @file     entry.asm
; @brief    32-bit code for jumping to 64-bit c
;==========================================

; multiboot2 flags
MB2_HEADER_MAGIC            equ 0xE85250D6
MB2_ARCH_I386               equ 0x0
MB2_BOOTLOADER_MAGIC        equ 0x36D76289

MB2_TAG_TYPE_FRAMEBUFFER    equ 5
MB2_FRAMEBUFFER_BPP32       equ 32

; gdt flags
GDT_ACCESS_RING0            equ (0 << 0)
GDT_ACCESS_READWRITE        equ (1 << 1)
GDT_ACCESS_EXECUTABLE       equ (1 << 3)
GDT_ACCESS_SEGMENT          equ (1 << 4)
GDT_ACCESS_PRESENT          equ (1 << 7)
GDT_LONG_MODE               equ (1 << 5)
GDT_GRANULARITY             equ (1 << 7)

; cpuid flags
CPUID_PSE                   equ (1 << 3)
CPUID_SSE                   equ (1 << 25)
CPUID_SSE2                  equ (1 << 26)
CPUID_LONG_MODE             equ (1 << 29)

; cpuid functions
CPUID_EXTENDED_MAX          equ 0x80000000
CPUID_EXTENDED_FEATURES     equ 0x80000001

FLAGS_ID                    equ (1 << 21)

; page table entry flags
PF_PRESENT                  equ (1 << 0)
PF_READ_WRITE               equ (1 << 1)
PF_PAGE_SIZE                equ (1 << 7)

; page sizes
PAGE_SIZE_LARGE             equ 0x200000

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

bits 32
section .boot.text
    ; remote 64 bit routine
    extern amd64_entry

    ; linker variables
    extern __lnk_kernel_end_physical

    ; globals
    global boot
    global page_table_l4

;==========================================
; @function     boot
; @brief        amd64 entry point for the kernel.
;               handles setting up of 64 bit (c) environment
;==========================================
boot:
    ; check multiboot magic
    cmp eax, MB2_BOOTLOADER_MAGIC
    jne .stop

    ; setup a stack
    mov esp,    stack.top
    and esp,    -16

    ; save multiboot structure
    push ebx ; multiboot structure

    call check_cpuid
    jz   .stop

    call check_paging
    jz   .stop

    call check_simd
    jz   .stop

    call check_long_mode
    jz   .stop

    call init_boot_page_table
    call enable_paging
    call enable_simd

    lgdt [boot_gdtr]

    pop edi ; multiboot structure (first argument)

    ; far jump to 64 bit code
    jmp boot_64_code_entry:amd64_entry

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
; @function     check_paging
; @brief        check if paging is supported
; @remarks      ZF=1 not supported, ZF=0 supported
;==========================================
check_paging:
    mov     eax, 1
    cpuid

    test    edx, CPUID_PSE
    jz      .fail
    
    xor     eax, eax
    inc     eax
    ret

.fail:
    xor     eax, eax
    ret

;==========================================
; @function     check_paging
; @brief        check if sse/sse2 is supported
; @remarks      ZF=1 not supported, ZF=0 supported
;==========================================
check_simd:
    mov     eax, 1
    cpuid

    test    edx, CPUID_SSE
    jz      .fail

    test    edx, CPUID_SSE2
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
; @brief        identity maps the kernel (?)
; @todo         switch to only id map the (c) bootloader
;==========================================
init_boot_page_table:
    mov     ebx,    page_table.l4
    mov     eax,    page_table.l3
    or      eax,    PF_PRESENT | PF_READ_WRITE
    mov     [ebx],  eax

    mov     ebx,    page_table.l3
    mov     eax,    page_table.l2
    or      eax,    PF_PRESENT | PF_READ_WRITE
    mov     [ebx],  eax

    mov     eax,    __lnk_kernel_end_physical
    add     eax,    PAGE_SIZE_LARGE - 1
    shr     eax,    21
    mov     ecx,    eax
    xor     edx,    edx

.loop:
    mov     eax,                edx
    shl     eax,                21
    or      eax,                PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE
    mov     ebx,                page_table.l2
    mov     [ebx + edx * 8],    eax
    inc     edx
    cmp     edx,                ecx
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

;==========================================
; @function     enable_simd
; @brief        enable simd (sse/sse2)
;==========================================
enable_simd:
    mov     eax, cr0
    or      eax, CR0_MP
    and     eax, ~CR0_EM
    mov     cr0, eax

    mov     eax, cr4
    or      eax, CR4_OSFXSR | CR4_OSXMMEXCPT
    mov     cr4, eax

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
.flags:         dw 0
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
.top:

; define a basic page table for jumping to 64 bit
; we only need to go 3 deep since we are only allocating 2mb pages
align 4096
page_table:
.l4:    resb 4096
.l3:    resb 4096
.l2:    resb 4096

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
.address:           dq zero_entry
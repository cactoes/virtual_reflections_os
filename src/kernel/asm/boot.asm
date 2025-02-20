;==========================================
; @file     boot.asm
; @brief    64-bit code for booting the kernel
;           also responsible for setting up the c/c++ env
;==========================================

section .text
    ; c functions
    extern kernel_entry
    extern call_constructors
    extern multiboot_magic

    ; loader variables
    extern __lnk_bss_size
    extern __lnk_bss_start
    extern __end_bss_keep

    ; globals
    global boot_kernel
    global KPML4T

bits 64
; @function          memzero
; @bried             wipes a memory region
; @param[in] rdi     target address
; @param[in] rsi     size
memzero:
    xor   rax,   rax
    mov   rcx,   rsi
    rep   stosb
    ret

boot_kernel:
    ; clean registers
    mov   ax,    0
    mov   ss,    ax
    mov   ds,    ax
    mov   es,    ax
    mov   fs,    ax
    mov   gs,    ax

    ; clear bss w/o clearing critical sections
    lea   rax,   [__end_bss_keep]
    lea   rbx,   __lnk_bss_start
    sub   rax,   rbx

    mov   rdi,   __end_bss_keep
    mov   rsi,   __lnk_bss_size
    sub   rsi,   rax
    call  memzero

    ; set kernel stack
    mov   rsp,    KSTACK_TOP
    and   rsp,    -16

    ; setup basic new page table (KPML4[0] -> KPDP[0] -> KPDT)
    mov   rax,      KPDP
    or    rax,      0b11
    mov   [KPML4], rax

    mov   rax,      KPD
    or    rax,      0b11
    mov   [KPDP],  rax

    ; setup remaining c++ stuff
    call call_constructors

    ; push multiboot struct to func
    mov rdi, multiboot_magic
    ; push kernel page table struct
    mov rsi, KPML4
    ; i hope everything has been setup, godspeed o7
    call kernel_entry

    ; infinite loop incase kernel exits
.loop:
    cli
    hlt
    jmp   .loop

section .bss
    align 4096
; page table for identity map (preloaded for the kernel to use)
KPML4:     resb 4096
KPDP:      resb 4096
KPD:       resb 4096

; 64 KB of kernel stack
KSTACK_BOTTOM: resb 4096 * 64
KSTACK_TOP:
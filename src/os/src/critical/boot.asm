;==========================================
; @file     boot.asm
; @brief    64-bit code for booring the kernel
;==========================================

section .text
    ; c functions
    extern kernel_main
    extern call_constructors

    ; loader variables
    extern bss_size
    extern bss_start
    extern end_bss_keep

    ; globals
    global loader64
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

loader64:
    ; clean registers
    mov   ax,    0
    mov   ss,    ax
    mov   ds,    ax
    mov   es,    ax
    mov   fs,    ax
    mov   gs,    ax

    ; clear bss w/o clearing critical sections
    lea   rax,   [end_bss_keep]
    lea   rbx,   bss_start
    sub   rax,   rbx

    mov   rdi,   end_bss_keep
    mov   rsi,   bss_size
    sub   rsi,   rax
    call  memzero

    ; set kernel stack
    mov   rsp,    KSTACK_TOP
    and   rsp,    -16

    ; setup basic new page table (KPML4T[0] -> KPDPT[0] -> KPDT)
    mov   rax,      KPDPT
    or    rax,      0b11
    mov   [KPML4T], rax

    mov   rax,      KPDT
    or    rax,      0b11
    mov   [KPDPT],  rax

    ; setup remaining c++ stuff
    call call_constructors

    ; i hope everything has been setup, godspeed o7
    call kernel_main

    ; infinite loop incase kernel exits
.loop:
    cli
    hlt
    jmp   .loop

section .bss
    align 4096
; page table for identity map (preloaded for the kernel to use)
KPML4T:     resb 4096
KPDPT:      resb 4096   
KPDT:       resb 4096

; 64 KB of kernel stack
KSTACK_BOTTOM: resb 4096 * 64
KSTACK_TOP:
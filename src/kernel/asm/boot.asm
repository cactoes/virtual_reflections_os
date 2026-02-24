;==========================================
; @file     boot.asm
; @brief    64-bit code for booting the kernel
;           also responsible for setting up the c/c++ env
;==========================================

section .text
    ; c functions
    extern kernel_entry
    extern MB_MAGIC

    ; loader variables
    extern __lnk_bss_size
    extern __lnk_bss_start
    extern __lnk_start_ctors
    extern __lnk_end_ctors
    extern __end_bss_keep

    ; globals
    global boot_kernel
    global KPML4T
    global KSTACK_TOP

bits 64

;==========================================
; @function          memzero
; @brief             wipes a memory region
; @param[in] rdi     target address
; @param[in] rsi     size
;==========================================
memzero:
    xor   rax,   rax
    mov   rcx,   rsi
    rep   stosb
    ret

; @function         call_constructors
; @brief
call_constructors:
    push    rbx
    mov     rsi,    __lnk_start_ctors

    .loop:
        cmp     rsi,    __lnk_end_ctors
        jge     .done

        mov     rdi,    [rsi]
        call    rdi
        add     rsi,    8
        jmp     .loop

    .done:
        pop     rbx
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

    ; setup remaining c++ stuff
    call call_constructors

    ; push multiboot struct to func
    mov rdi, MB_MAGIC
    ; push kernel page table struct
    mov rsi, cr3
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
KPML4T:     resb 4096
KPDPT:      resb 4096
KPDT:       resb 4096

; 2 MB of kernel stack
KSTACK_BOTTOM: resb 1024 * 1024 * 2
KSTACK_TOP:
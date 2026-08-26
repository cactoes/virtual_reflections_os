;==========================================
; @file     stub.asm
; @brief    amd64 stubs
;==========================================

bits 64
section .text
    extern amd64_syscall_dispatch
    extern amd64_interrupt_dispatch
    extern fpu_scratch

    global amd64_memcpy_impl
    global amd64_memset_impl

    global amd64_syscall_stub
    global amd64_isr_stub_table

;==========================================
; @macro    push_all_regs
; @brief    pushes all gpr's to the stack
;==========================================
%macro push_all_regs 0
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
%endmacro

;==========================================
; @macro    pop_all_regs
; @brief    pops all gpr's from the stack
;==========================================
%macro pop_all_regs 0
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
%endmacro

;==========================================
; @macro                    isr_stub
; @brief                    function to handle incoming interrupt
;==========================================
%macro isr_stub 1
isr_stub_%+%1:
    cli

%if %1 != 8 && %1 != 10 && %1 != 11 && %1 != 12 && %1 != 13 && %1 != 14 && %1 != 17 && %1 != 30
    push 0
%endif

    test    qword [rsp + 16], 3
    jz      .no_swapgs_in_%1
    swapgs

    .no_swapgs_in_%1:

    push_all_regs

    fxsave [rel fpu_scratch]

    ; store isr code
    mov     rdi, %1
    ; store pointer to the stack
    mov     rsi, rsp

    ; re-align stack
    sub     rsp, 8

    ; call the interrupt handler
    call    amd64_interrupt_dispatch

    ; update / restore stack pointer
    mov     rsp, rax

    fxrstor [rel fpu_scratch]

    pop_all_regs

    test    qword[rsp + 16], 3
    jz      .no_swapgs_out_%1
    swapgs

    .no_swapgs_out_%1:

    ; skip error code
    add     rsp, 8

    ; return from interrupt
    iretq
%endmacro

;==========================================
; @function                 amd64_syscall_stub
; @brief                    the syscall handler that gets invoked
;==========================================
amd64_syscall_stub:
    cli
    swapgs

    mov [gs:8], rsp
    mov rsp, [gs:0]
    push qword [gs:8]

    push rcx
    push r11

    push rdi
    push rsi
    push rdx
    push r8
    push r9
    push r10
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov rdi, rax
    mov rsi, rsp

    ; TODO
    ; sub rsp, 512
    ; and rsp, -16
    ; fxsave [rsp]

    ; sti
    call amd64_syscall_dispatch
    ; cli

    ; fxrstor [rsp]
    ; add rsp, 512

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rsi
    pop rdi

    pop r11
    pop rcx

    pop rsp

    swapgs
    o64 sysret

;==========================================
; @brief    cpu interrupts
;==========================================
isr_stub 0  ; isr 0
isr_stub 1  ; isr 1
isr_stub 2  ; isr 2
isr_stub 3  ; isr 3
isr_stub 4  ; isr 4
isr_stub 5  ; isr 5
isr_stub 6  ; isr 6
isr_stub 7  ; isr 7
isr_stub 8  ; isr 8
isr_stub 9  ; isr 9
isr_stub 10 ; isr 10
isr_stub 11 ; isr 11
isr_stub 12 ; isr 12
isr_stub 13 ; isr 13
isr_stub 14 ; isr 14
isr_stub 15 ; isr 15
isr_stub 16 ; isr 16
isr_stub 17 ; isr 17
isr_stub 18 ; isr 18
isr_stub 19 ; isr 19
isr_stub 20 ; isr 20
isr_stub 21 ; isr 21 last basic interupt
isr_stub 22 ; isr 22
isr_stub 23 ; isr 23
isr_stub 24 ; isr 24
isr_stub 25 ; isr 25
isr_stub 26 ; isr 26
isr_stub 27 ; isr 27
isr_stub 28 ; isr 28
isr_stub 29 ; isr 29
isr_stub 30 ; isr 30
isr_stub 31 ; isr 31

;==========================================
; @brief    hardware interrupts
;==========================================
isr_stub 32 ; irq0  pit
isr_stub 33 ; irq1  keyboard
isr_stub 34 ; irq2  cascade
isr_stub 35 ; irq3  com2
isr_stub 36 ; irq4  com1
isr_stub 37 ; irq5  lpt2
isr_stub 38 ; irq6  floppydisk
isr_stub 39 ; irq7  lpt1
isr_stub 40 ; irq8  cmos rt clock
isr_stub 41 ; irq9  free
isr_stub 42 ; irq10 free
isr_stub 43 ; irq11 free
isr_stub 44 ; irq12 ps2 mouse
isr_stub 45 ; irq13 fpu
isr_stub 46 ; irq14 primary ata harddisk
isr_stub 47 ; irq15 secondary ata harddisk

;==========================================
; @brief    other interrupts 48-256
;==========================================
%assign  vec 48
%rep  (257-48+1)
    isr_stub vec
%assign  vec vec+1
%endrep

;==========================================
; @brief    interrupt array, stores all interrupts
;==========================================
amd64_isr_stub_table:
%assign i 0 
%rep    257
    dq isr_stub_%+i
%assign i i+1 
%endrep

;==========================================
; @function                 amd64_memset_impl
; @brief                    ?
; @param rdi, in            ?
; @param rsi, in            ?
; @param rdx, in            ?
;==========================================
align 16
amd64_memset_impl:
    mov     rax, rdi
    test    rdx, rdx
    jz      .done

    movzx   r8, sil
    imul    r8, 0x0101010101010101
    
    cmp     rdx, 16
    jb      .small_set

    movq    xmm0, r8
    punpcklqdq xmm0, xmm0

    cmp     rdx, 128
    jb      .sse_unaligned

    mov     rcx, rdi
    and     rcx, 15
    jz      .sse_aligned
    
    mov     r9, 16
    sub     r9, rcx
    sub     rdx, r9
    
    mov     rax, r8
.align_loop:
    mov     [rdi], al
    inc     rdi
    dec     r9
    jnz     .align_loop
    
    mov     rax, rdi

.sse_aligned:
    mov     rcx, rdx
    shr     rcx, 7
    jz      .sse_remainder

.sse_large_loop:
    movdqa  [rdi], xmm0
    movdqa  [rdi + 16], xmm0
    movdqa  [rdi + 32], xmm0
    movdqa  [rdi + 48], xmm0
    movdqa  [rdi + 64], xmm0
    movdqa  [rdi + 80], xmm0
    movdqa  [rdi + 96], xmm0
    movdqa  [rdi + 112], xmm0
    
    add     rdi, 128
    dec     rcx
    jnz     .sse_large_loop
    
    and     rdx, 127

.sse_remainder:
.sse_unaligned:
    mov     rcx, rdx
    shr     rcx, 4
    jz      .tail

.sse_loop:
    movdqu  [rdi], xmm0
    add     rdi, 16
    dec     rcx
    jnz     .sse_loop
    
    and     rdx, 15

.tail:
    test    rdx, rdx
    jz      .done
    
    cmp     rdx, 8
    jb      .tail_small
    
    mov     [rdi], r8
    add     rdi, 8
    sub     rdx, 8

.tail_small:
    test    rdx, rdx
    jz      .done
    mov     rcx, rdx
    mov     rax, r8
    rep     stosb
    jmp     .done_restore

.small_set:
    cmp     rdx, 8
    jb      .tiny_set
    
    mov     [rdi], r8
    add     rdi, 8
    sub     rdx, 8
    
.tiny_set:
    test    rdx, rdx
    jz      .done
    mov     rcx, rdx
    mov     rax, r8
    rep     stosb
    jmp     .done_restore

.done_restore:
    mov     rax, rdi
    sub     rax, rdx
.done:
    ret

;==========================================
; @function                 amd64_memcpy_impl
; @brief                    ?
; @param rdi, in            ?
; @param rsi, in            ?
; @param rdx, in            ?
;==========================================
align 16
amd64_memcpy_impl:
    mov     rax, rdi
    test    rdx, rdx
    jz      .done
    
    cmp     rdx, 16
    jb      .small_copy

    cmp     rdx, 128
    jb      .sse_unaligned

    mov     rcx, rdi
    and     rcx, 15
    jz      .sse_aligned
    
    mov     r8, 16
    sub     r8, rcx
    sub     rdx, r8
    
.align_loop:
    mov     cl, [rsi]
    mov     [rdi], cl
    inc     rsi
    inc     rdi
    dec     r8
    jnz     .align_loop

.sse_aligned:
    mov     rcx, rdx
    shr     rcx, 7
    jz      .sse_remainder

.sse_large_loop:
    movdqu  xmm0, [rsi]
    movdqu  xmm1, [rsi + 16]
    movdqu  xmm2, [rsi + 32]
    movdqu  xmm3, [rsi + 48]
    movdqu  xmm4, [rsi + 64]
    movdqu  xmm5, [rsi + 80]
    movdqu  xmm6, [rsi + 96]
    movdqu  xmm7, [rsi + 112]
    
    movdqa  [rdi], xmm0
    movdqa  [rdi + 16], xmm1
    movdqa  [rdi + 32], xmm2
    movdqa  [rdi + 48], xmm3
    movdqa  [rdi + 64], xmm4
    movdqa  [rdi + 80], xmm5
    movdqa  [rdi + 96], xmm6
    movdqa  [rdi + 112], xmm7
    
    add     rsi, 128
    add     rdi, 128
    dec     rcx
    jnz     .sse_large_loop
    
    and     rdx, 127

.sse_remainder:
.sse_unaligned:
    mov     rcx, rdx
    shr     rcx, 4
    jz      .tail

.sse_loop:
    movdqu  xmm0, [rsi]
    movdqu  [rdi], xmm0
    add     rsi, 16
    add     rdi, 16
    dec     rcx
    jnz     .sse_loop
    
    and     rdx, 15

.tail:
    test    rdx, rdx
    jz      .done
    
    cmp     rdx, 8
    jb      .tail_small
    
    mov     rcx, [rsi]
    mov     [rdi], rcx
    add     rsi, 8
    add     rdi, 8
    sub     rdx, 8

.tail_small:
    test    rdx, rdx
    jz      .done
    mov     rcx, rdx
    rep     movsb
    jmp     .done

.small_copy:
    cmp     rdx, 8
    jb      .tiny_copy
    
    mov     rcx, rdx
    shr     rcx, 3
    rep     movsq
    
    and     rdx, 7
    jz      .done

.tiny_copy:
    mov     rcx, rdx
    rep     movsb

.done:
    ret
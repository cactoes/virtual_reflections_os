;==========================================
; @file     amd64_isr.asm
; @brief    
; @note     this code IS generated (claude sonnet 5.5)
;==========================================

bits 64
    extern amd64_interrupt_stub
    global isr_stub_table

%macro ISR_NOERR 1
isr_stub_%+%1:
    push 0
    push %1
    jmp  isr_common
%endmacro

%macro ISR_ERR 1
isr_stub_%+%1:
    push %1
    jmp  isr_common
%endmacro

section .text

%assign i 0
%rep 256
    %if i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21 || i == 29 || i == 30
        ISR_ERR i
    %else
        ISR_NOERR i
    %endif
    %assign i i+1
%endrep

isr_common:
    test qword [rsp + 24], 3
    jz   .entry_from_kernel
    swapgs
.entry_from_kernel:

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov  rdi, rsp
    cld
    call amd64_interrupt_stub
    mov  rsp, rax

    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  r11
    pop  r10
    pop  r9
    pop  r8
    pop  rbp
    pop  rdi
    pop  rsi
    pop  rdx
    pop  rcx
    pop  rbx
    pop  rax
    add  rsp, 16

    test qword [rsp + 8], 3
    jz   .exit_to_kernel
    swapgs
.exit_to_kernel:
    iretq

section .rodata
isr_stub_table:
%assign i 0
%rep 256
    dq isr_stub_%+i
    %assign i i+1
%endrep
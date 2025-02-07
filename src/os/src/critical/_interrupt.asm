;==========================================
; @file     _interrupt.asm
; @brief    interrupt pre handler code
;==========================================

bits 64
    extern kernel_interrupt_handler

; %macro isr_err_stub 1
; isr_stub_%+%1:
;     mov rdi, %1
;     call kernel_interrupt_handler
;     iretq
; %endmacro

; %macro isr_no_err_stub 1
; isr_stub_%+%1:
;     mov rdi, %1
;     call kernel_interrupt_handler
;     iretq
; %endmacro

%macro isr_stub 1
isr_stub_%+%1:

    ; store cpu state
    pushfq
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push rbp
    push rsp

    ; store isr code
    mov rdi, %1
    ; store pointer to the stack
    mov rsi, rsp
    ; call interrupt handler
    call kernel_interrupt_handler
    ; manually align stack
    add rax, 8
    ; update / restore stack pointer
    mov rsp, rax

    ; update / restore cpu state
    pop rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    popfq

    ; return from interrupt
    iretq
%endmacro

; cpu
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
isr_stub 21 ; isr 21
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

; hardware
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

global isr_stub_table

isr_stub_table:
%assign i 0 
%rep    48
    dq isr_stub_%+i
%assign i i+1 
%endrep
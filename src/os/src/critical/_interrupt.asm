bits 64
    extern kernel_interrupt_handler

%macro isr_err_stub 1
isr_stub_%+%1:
    mov rdi, %1
    call kernel_interrupt_handler
    iretq
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    mov rdi, %1
    call kernel_interrupt_handler
    iretq
%endmacro

; cpu
isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

; hardware
isr_no_err_stub 32 ; IRQ1 PIT
isr_no_err_stub 33 ; IRQ2 keyboard
isr_no_err_stub 34
isr_no_err_stub 35
isr_no_err_stub 36
isr_no_err_stub 37
isr_no_err_stub 38
isr_no_err_stub 39
isr_no_err_stub 40
isr_no_err_stub 41
isr_no_err_stub 42
isr_no_err_stub 43
isr_no_err_stub 44  ; IRQ12 mouse
isr_no_err_stub 45
isr_no_err_stub 46
isr_no_err_stub 47

global isr_stub_table

isr_stub_table:
%assign i 0 
%rep    48
    dq isr_stub_%+i
%assign i i+1 
%endrep
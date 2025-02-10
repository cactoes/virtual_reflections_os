;==========================================
; @file     _kernel.asm
; @brief    kernel assembly stubs
;==========================================

bits 64
section .text
    global __dump_cpu

;==========================================
; @brief                    function to dump the current cpu_state
; @param rdi, buffer        pointer to cpu struct
; @remarks                  no registers destroyed
;==========================================
__dump_cpu:
    mov rbx, rsp

    ; store cpu state on stack
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

    ; Copy the CPU state from stack to buffer
    mov rsi, rsp    ; source cpu_state aka current stack
    mov rcx, 16     ; 16 registers
    cld
    rep movsq       ; copy into rdi (buffer)

    ; restore stack pointer
    mov rsp, rbx

    ; return to caller
    ret
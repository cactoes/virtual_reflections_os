;==========================================
; @file     amd64_syscall_stubs.asm
; @brief    
; @note     this code IS generated (claude sonnet 5.5)
;==========================================

bits 64
    extern amd64_syscall_dispatch
    global amd64_syscall_stub

amd64_syscall_stub:
    swapgs

    mov  [gs:8], rsp
    mov  rsp, [gs:0]

    push qword [gs:8]
    push r11
    push rcx
    push rax
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

    mov  rdi, rsp
    cld
    call amd64_syscall_dispatch

    cli

    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  rbp
    pop  rbx
    pop  r10
    pop  r9
    pop  r8
    pop  rdx
    pop  rsi
    pop  rdi
    pop  rax
    pop  rcx
    pop  r11
    pop  rsp

    swapgs
    o64 sysret
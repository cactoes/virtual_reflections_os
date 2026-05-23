global _entry
extern main

section .text
_entry:
    call main

    ; syscall exit (user) process
    ; if we dont do this we just crash the thread
    mov rax, 0
    syscall
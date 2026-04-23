global _entry
extern main

section .text
_entry:
    call main

    ; syscall exit (user) process
    mov rax, 0
    syscall
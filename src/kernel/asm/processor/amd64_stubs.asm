;==========================================
; @file     amd64_stubs.asm
; @brief    generic amd64 based function instructions
; @note     this code IS generated (claude sonnet 5.5)
;==========================================

%define USE_ERMS            1
%define MEMSET_REP_MIN      256
%define MEMCPY_REP_MIN      512
%define MEMMOVE_REP_MIN     512

bits 64
section .text
    global __memzero_impl
    global __memset_impl
    global __memcpy_impl
    global __memeq_impl
    global __memmove_impl

;==========================================
; @function
; rdi = dst, rsi = size
;==========================================
align 16
__memzero_impl:
    mov     rdx, rsi
    xor     eax, eax
    mov     r9, rdi
    jmp     __fill_common

;==========================================
; @function
;   rdi = dst, sil = val, rdx = size
;==========================================
align 16
__memset_impl:
    movzx   esi, sil
    mov     rax, 0x0101010101010101
    imul    rax, rsi
    mov     r9, rdi

__fill_common:
    cmp     rdx, 16
    ja      .above16
    cmp     rdx, 8
    jae     .s8_16
    cmp     rdx, 4
    jae     .s4_7
    test    rdx, rdx
    jz      .done
    mov     [rdi], al
    mov     [rdi + rdx - 1], al
    cmp     rdx, 2
    jbe     .done
    mov     [rdi + 1], al
.done:
    mov     rax, r9
    ret

.s4_7:
    mov     [rdi], eax
    mov     [rdi + rdx - 4], eax
    jmp     .done

.s8_16:
    mov     [rdi], rax
    mov     [rdi + rdx - 8], rax
    jmp     .done

.above16:
    cmp     rdx, 32
    ja      .above32
    mov     [rdi], rax
    mov     [rdi + 8], rax
    mov     [rdi + rdx - 16], rax
    mov     [rdi + rdx - 8], rax
    jmp     .done

.above32:
    cmp     rdx, MEMSET_REP_MIN
    jae     .large
    lea     rcx, [rdi + rdx - 32]
.loop32:
    mov     [rdi], rax
    mov     [rdi + 8], rax
    mov     [rdi + 16], rax
    mov     [rdi + 24], rax
    add     rdi, 32
    cmp     rdi, rcx
    jb      .loop32
    mov     [rcx], rax
    mov     [rcx + 8], rax
    mov     [rcx + 16], rax
    mov     [rcx + 24], rax
    jmp     .done

.large:
%if USE_ERMS
    mov     rcx, rdx
    rep stosb
%else
    mov     [rdi + rdx - 8], rax
    mov     rcx, rdx
    shr     rcx, 3
    rep stosq
%endif
    jmp     .done

;==========================================
; @function
;   rdi = dst, rsi = src, rdx = size
;==========================================
align 16
__memcpy_impl:
    mov     rax, rdi
    cmp     rdx, 16
    ja      .above16
    cmp     rdx, 8
    jae     .c8_16
    cmp     rdx, 4
    jae     .c4_7
    test    rdx, rdx
    jz      .ret
    movzx   ecx, byte [rsi]
    movzx   r8d, byte [rsi + rdx - 1]
    cmp     rdx, 2
    jbe     .c1_2
    movzx   r9d, byte [rsi + 1]
    mov     [rdi + 1], r9b
.c1_2:
    mov     [rdi], cl
    mov     [rdi + rdx - 1], r8b
.ret:
    ret

.c4_7:
    mov     ecx, [rsi]
    mov     r8d, [rsi + rdx - 4]
    mov     [rdi], ecx
    mov     [rdi + rdx - 4], r8d
    ret

.c8_16:
    mov     rcx, [rsi]
    mov     r8,  [rsi + rdx - 8]
    mov     [rdi], rcx
    mov     [rdi + rdx - 8], r8
    ret

.above16:
    cmp     rdx, 32
    ja      .above32
    mov     rcx, [rsi]
    mov     r8,  [rsi + 8]
    mov     r9,  [rsi + rdx - 16]
    mov     r10, [rsi + rdx - 8]
    mov     [rdi], rcx
    mov     [rdi + 8], r8
    mov     [rdi + rdx - 16], r9
    mov     [rdi + rdx - 8], r10
    ret

.above32:
    cmp     rdx, MEMCPY_REP_MIN
    jae     .large
    lea     r11, [rdi + rdx - 32]
    lea     rdx, [rsi + rdx - 32]
.loop32:
    mov     rcx, [rsi]
    mov     r8,  [rsi + 8]
    mov     r9,  [rsi + 16]
    mov     r10, [rsi + 24]
    mov     [rdi], rcx
    mov     [rdi + 8], r8
    mov     [rdi + 16], r9
    mov     [rdi + 24], r10
    add     rsi, 32
    add     rdi, 32
    cmp     rdi, r11
    jb      .loop32
    mov     rcx, [rdx]
    mov     r8,  [rdx + 8]
    mov     r9,  [rdx + 16]
    mov     r10, [rdx + 24]
    mov     [r11], rcx
    mov     [r11 + 8], r8
    mov     [r11 + 16], r9
    mov     [r11 + 24], r10
    ret

.large:
%if USE_ERMS
    mov     rcx, rdx
    rep movsb
%else
    mov     r8, [rsi + rdx - 8]
    lea     r9, [rdi + rdx - 8]
    mov     rcx, rdx
    shr     rcx, 3
    rep movsq
    mov     [r9], r8
%endif
    ret

;==========================================
; @function
;   rdi = a, rsi = b, rdx = size
;   returns 1 (equal) / 0 (different) in eax
;==========================================
align 16
__memeq_impl:
    cmp     rdx, 16
    ja      .above16
    cmp     rdx, 8
    jae     .e8_16
    cmp     rdx, 4
    jae     .e4_7
    test    rdx, rdx
    jz      .equal
    movzx   eax, byte [rdi]
    movzx   ecx, byte [rsi]
    sub     eax, ecx
    movzx   ecx, byte [rdi + rdx - 1]
    movzx   r8d, byte [rsi + rdx - 1]
    sub     ecx, r8d
    or      eax, ecx
    cmp     rdx, 3
    jne     .result
    movzx   ecx, byte [rdi + 1]
    movzx   r8d, byte [rsi + 1]
    sub     ecx, r8d
    or      eax, ecx
    jmp     .result

.equal:
    mov     eax, 1
    ret

.e4_7:
    mov     eax, [rdi]
    xor     eax, [rsi]
    mov     ecx, [rdi + rdx - 4]
    xor     ecx, [rsi + rdx - 4]
    or      eax, ecx
    jmp     .result

.e8_16:
    mov     rax, [rdi]
    xor     rax, [rsi]
    mov     rcx, [rdi + rdx - 8]
    xor     rcx, [rsi + rdx - 8]
    or      rax, rcx
    jmp     .result

.above16:
    cmp     rdx, 32
    ja      .above32
    mov     rax, [rdi]
    xor     rax, [rsi]
    mov     rcx, [rdi + 8]
    xor     rcx, [rsi + 8]
    mov     r8,  [rdi + rdx - 16]
    xor     r8,  [rsi + rdx - 16]
    mov     r9,  [rdi + rdx - 8]
    xor     r9,  [rsi + rdx - 8]
    or      rax, rcx
    or      r8, r9
    or      rax, r8
    jmp     .result

.above32:
    lea     r10, [rdi + rdx - 32]
    sub     rsi, rdi
.loop32:
    mov     rax, [rdi]
    xor     rax, [rdi + rsi]
    mov     rcx, [rdi + 8]
    xor     rcx, [rdi + rsi + 8]
    mov     r8,  [rdi + 16]
    xor     r8,  [rdi + rsi + 16]
    mov     r9,  [rdi + 24]
    xor     r9,  [rdi + rsi + 24]
    or      rax, rcx
    or      r8, r9
    or      rax, r8
    jnz     .differ
    add     rdi, 32
    cmp     rdi, r10
    jb      .loop32
    mov     rax, [r10]
    xor     rax, [r10 + rsi]
    mov     rcx, [r10 + 8]
    xor     rcx, [r10 + rsi + 8]
    mov     r8,  [r10 + 16]
    xor     r8,  [r10 + rsi + 16]
    mov     r9,  [r10 + 24]
    xor     r9,  [r10 + rsi + 24]
    or      rax, rcx
    or      r8, r9
    or      rax, r8

.result:
    test    rax, rax
    sete    al
    movzx   eax, al
    ret

.differ:
    xor     eax, eax
    ret

;==========================================
; @function
;   rdi = dst, rsi = src, rdx = size
;   returns dst in rax
;==========================================
align 16
__memmove_impl:
    mov     rax, rdi
.dispatch:
    cmp     rdx, 16
    ja      .above16
    cmp     rdx, 8
    jae     .m8_16
    cmp     rdx, 4
    jae     .m4_7
    test    rdx, rdx
    jz      .ret
    movzx   ecx, byte [rsi]
    movzx   r8d, byte [rsi + rdx - 1]
    cmp     rdx, 2
    jbe     .m1_2
    movzx   r9d, byte [rsi + 1]
    mov     [rdi + 1], r9b
.m1_2:
    mov     [rdi], cl
    mov     [rdi + rdx - 1], r8b
.ret:
    ret
 
.m4_7:
    mov     ecx, [rsi]
    mov     r8d, [rsi + rdx - 4]
    mov     [rdi], ecx
    mov     [rdi + rdx - 4], r8d
    ret
 
.m8_16:
    mov     rcx, [rsi]
    mov     r8,  [rsi + rdx - 8]
    mov     [rdi], rcx
    mov     [rdi + rdx - 8], r8
    ret
 
.above16:
    cmp     rdx, 32
    ja      .above32
    mov     rcx, [rsi]
    mov     r8,  [rsi + 8]
    mov     r9,  [rsi + rdx - 16]
    mov     r10, [rsi + rdx - 8]
    mov     [rdi], rcx
    mov     [rdi + 8], r8
    mov     [rdi + rdx - 16], r9
    mov     [rdi + rdx - 8], r10
    ret
 
.above32:
    cmp     rdi, rsi
    je      .ret
    mov     rcx, rdi
    sub     rcx, rsi
    cmp     rcx, rdx
    jb      .backward
 
.forward:
    cmp     rdx, MEMMOVE_REP_MIN
    jae     .large
.loop32f:
    mov     rcx, [rsi]
    mov     r8,  [rsi + 8]
    mov     r9,  [rsi + 16]
    mov     r10, [rsi + 24]
    mov     [rdi], rcx
    mov     [rdi + 8], r8
    mov     [rdi + 16], r9
    mov     [rdi + 24], r10
    add     rsi, 32
    add     rdi, 32
    sub     rdx, 32
    cmp     rdx, 32
    jae     .loop32f
    jmp     .dispatch
 
.large:
%if USE_ERMS
    mov     rcx, rdx
    rep movsb
%else
    mov     r8, [rsi + rdx - 8]
    lea     r9, [rdi + rdx - 8]
    mov     rcx, rdx
    shr     rcx, 3
    rep movsq
    mov     [r9], r8
%endif
    ret
 
.backward:
.loop32b:
    mov     rcx, [rsi + rdx - 8]
    mov     r8,  [rsi + rdx - 16]
    mov     r9,  [rsi + rdx - 24]
    mov     r10, [rsi + rdx - 32]
    mov     [rdi + rdx - 8], rcx
    mov     [rdi + rdx - 16], r8
    mov     [rdi + rdx - 24], r9
    mov     [rdi + rdx - 32], r10
    sub     rdx, 32
    cmp     rdx, 32
    jae     .loop32b
    jmp     .dispatch

section .note.GNU-stack noalloc noexec nowrite progbits
global amd64_memset_impl
bits 64

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
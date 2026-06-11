global amd64_memcpy_impl
bits 64

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
; Compile with NASM:
;   $ nasm ram.asm -o ram.bin

; This is where the RAM program is loaded
[BITS 64]
org 0x10000

Entry:
    ; Do a simple read
	mov rax, [0x10000]
    
    ; Test the stack
    push rax
    pop r10
    mov rdx, r10

Host.PageManipulation1:
    hlt                     ; Stop for a moment to let the host manipulate the page tables
    mov rsi, 0x100000000    ; Test new memory allocated by the host
    mov rax, [rsi]

Host.PageManipulation2:
    hlt                     ; Let the host manipulate the page tables again
    mov rax, [rsi]          ; Test modification by the host

; TODO: check for support before each test
; - check relevant CPUID bits
; - notify host through a register

SSE.Enable:
    hlt
    mov rax, cr0
    and ax, 0xFFFB          ; Clear coprocessor emulation CR0.EM
    or ax, 0x2              ; Set coprocessor monitoring  CR0.MP
    mov cr0, rax
    mov rax, cr4
    or eax, (0b11 << 9)     ; Set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
    mov cr4, rax

MMX.Test:    
    emms                    ; Clear MMX state

    movq mm0, [mmx.v1]      ; Load first vector
    movq mm1, [mmx.v2]      ; Load second vector
    movq mm2, [mmx.v3]      ; Load third vector

    pmullw mm0, mm1         ; Multiply v1 and v2, store low words into mm0
    paddw mm0, mm2          ; Add v3 to result

    movq [mmx.r], mm0       ; Write result to memory
    movq rax, mm0           ; Copy result to RAX
    lea rsi, [mmx.r]        ; Put address of result into RSI

    hlt                     ; Let the host check the results
    emms                    ; Be a good citizen and clear MMX state after we're done

SSE.Test:
    movups xmm0, [sse.v1]   ; Load first vector
    movups xmm1, [sse.v2]   ; Load second vector

    addps xmm0, xmm1        ; Add v1 and v2, store result in xmm0
    mulps xmm0, xmm1        ; Multiply result by v2, store result in xmm0
    subps xmm0, xmm1        ; Subtract v2 from result, store result in xmm0

    movups [sse.r], xmm0    ; Write result to memory
    movq rax, xmm0          ; Copy low 64 bits of result to RAX
    lea rsi, [sse.r]        ; Put address of result into RSI

    hlt                     ; Let the host check the result

SSE2.Test:
    movupd xmm0, [sse2.v1]  ; Load first vector
    movupd xmm1, [sse2.v2]  ; Load second vector

    addpd xmm0, xmm1        ; Add v1 and v2, store result in xmm0
    mulpd xmm0, xmm1        ; Multiply result by v2, store result in xmm0
    subpd xmm0, xmm1        ; Subtract v2 from result, store result in xmm0

    movupd [sse2.r], xmm0   ; Write result to memory
    movq rax, xmm0          ; Copy low 64 bits of result to RAX
    lea rsi, [sse2.r]       ; Put address of result into RSI

    hlt                     ; Let the host check the result

SSE3.Test:
    movupd xmm0, [sse3.v1]  ; Load first vector
    movupd xmm1, [sse3.v2]  ; Load second vector
    
    haddpd xmm0, xmm1       ; Horizontally add the values in xmm1 and xmm0, store results in xmm0
  
    movupd [sse3.r], xmm0   ; Write result to memory
    movq rax, xmm0          ; Copy low 64 bits of result to RAX
    lea rsi, [sse3.r]       ; Put address of result into RSI
  
    hlt                     ; Let the host check the result

SSSE3.Test:
    movupd xmm0, [ssse3.v]  ; Load vector
    
    pabsd xmm0, xmm0        ; Compute absolute values
    phaddd xmm1, xmm0       ; Horizontally add each pair of consecutive 32-bit integers from xmm0, pack results into xmm1
    
    movupd [ssse3.r], xmm1  ; Write result to memory
    movq rax, xmm1          ; Copy low 64 bits of result to RAX
    lea rsi, [ssse3.r]      ; Put address of result into RSI
    
    hlt

SSE4.Test:   ; Includes SSE4.1 and SSE4.2
    ; TODO: write test
    hlt                     ; Let the host check the result

AVX.Enable:
    mov rax, cr4
    or eax, (1 << 18)       ; Set CR4.OSXSAVE
    mov cr4, rax
    xor rcx, rcx
    xgetbv                  ; Load XCR0 register
    or eax, 7               ; Set AVX, SSE, X87 bits
    xsetbv                  ; Save back to XCR0

AVX.Test:
    vzeroall                ; Clear YMM registers
    vmovups ymm0, [avx.v1]  ; Load v1 into ymm0
    vmovups ymm1, [avx.v2]  ; Load v2 into ymm1
    vmovups ymm2, [avx.v3]  ; Load v3 into ymm2

    vaddps ymm3, ymm0, ymm1 ; Add ymm0 and ymm1, store result in ymm3
    vmulps ymm3, ymm3, ymm2 ; Multiply ymm3 with ymm2, store result in ymm3

    vmovups [avx.r], ymm3   ; Write result to memory
    vmovq rax, xmm3         ; Copy low 64 bits of result to RAX
    lea rsi, [avx.r]        ; Put address of result into RSI

    hlt                     ; Let the host check the result

FMA3.Test:
    ; TODO: write test
    hlt                     ; Let the host check the result

AVX2.Test:
    ; TODO: write test
    hlt                     ; Let the host check the result

    ; We're done
Die:
    cli
    hlt
    jmp Die
    
ALIGN 16
    ; Data for MMX test
    mmx.v1: dw 5, 10, 15, 20
    mmx.v2: dw 2, 2, 2, 2
    mmx.v3: dw 1, 2, 3, 4
    mmx.r:  resw 4

    ; Data for SSE test
    sse.v1: dd 1.1, 2.2, 3.3, 4.4
    sse.v2: dd 5.5, 6.6, 7.7, 8.8
    sse.r:  resd 4

    ; Data for SSE2 test
    sse2.v1: dq 1.1, 2.2
    sse2.v2: dq 3.3, 4.4
    sse2.r:  resq 2

    ; Data for SSE3 test
    sse3.v1: dq 1.5, 2.5
    sse3.v2: dq 2.5, -0.5
    sse3.r:  resq 2

    ; Data for SSSE3 test
    ssse3.v: dd 1234, -5678, -1234, 5678
    ssse3.r: resd 4

    ; Data for SSE4 test
    ; TODO

    ; Data for AVX test
    avx.v1: dd 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5
    avx.v2: dd 8.5, 7.5, 6.5, 5.5, 4.5, 3.5, 2.5, 1.5
    avx.v3: dd 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5
    avx.r:  resd 8

    ; Data for FMA3 test
    ; TODO

    ; Data for AVX2 test
    ; TODO


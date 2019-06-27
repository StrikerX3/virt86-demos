; Compile with NASM:
;   $ nasm ram.asm -o ram.bin

; This is where the RAM program is loaded
[BITS 64]
org 0x10000

; Define bits for floating point extension tests
%define FPTEST_MMX    (1 << 0)
%define FPTEST_SSE    (1 << 1)
%define FPTEST_SSE2   (1 << 2)
%define FPTEST_SSE3   (1 << 3)
%define FPTEST_SSSE3  (1 << 4)
%define FPTEST_SSE4   (1 << 5)
%define FPTEST_XSAVE  (1 << 6)
%define FPTEST_AVX    (1 << 7)
%define FPTEST_FMA3   (1 << 8)
%define FPTEST_AVX2   (1 << 9)

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
    hlt                     ; Stop here

FPTests.Init:
    xor r15, r15            ; R15 will contain bits indicating which features the guest supports and will test

    mov eax, 1              ; Read CPUID page 1
    xor rcx, rcx
    cpuid

    test edx, (1 << 23)     ; MMX bit
    jz FPTests.Init.End
    or r15, FPTEST_MMX

    test edx, (1 << 25)     ; SSE bit
    jz FPTests.Init.End
    or r15, FPTEST_SSE

    test edx, (1 << 26)     ; SSE2 bit
    jz FPTests.Init.End
    or r15, FPTEST_SSE2

    test ecx, (1 << 0)      ; SSE3 bit
    jz FPTests.Init.End
    or r15, FPTEST_SSE3

    test ecx, (1 << 9)      ; SSSE3 bit
    jz FPTests.Init.End
    or r15, FPTEST_SSSE3

    test ecx, (0b11 << 19)  ; SSE4_1 and SSE4_2 bits
    jz FPTests.Init.End
    or r15, FPTEST_SSE4

    test ecx, (1 << 26)     ; XSAVE bit
    jz FPTests.Init.End
    or r15, FPTEST_XSAVE

    test ecx, (1 << 28)     ; AVX bit
    jz FPTests.Init.End
    or r15, FPTEST_AVX

    test ecx, (1 << 12)     ; FMA3 bit
    jz FPTests.Init.End
    or r15, FPTEST_FMA3

    mov eax, 7              ; Read CPUID page 7
    xor ecx, ecx
    cpuid

    test ebx, (1 << 5)      ; AVX2 bit
    jz FPTests.Init.End
    or r15, FPTEST_AVX2

FPTests.Init.End:
    hlt

MMX.Test:
    test r15, FPTEST_MMX    ; Check if MMX test is enabled
    jz FPTests.End          ; Leave tests if disabled

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

SSE.Enable:
    test r15, FPTEST_SSE    ; Check if SSE test is enabled
    jz FPTests.End          ; Leave tests if disabled

    mov rax, cr0
    and ax, 0xFFFB          ; Clear coprocessor emulation CR0.EM
    or ax, 0x2              ; Set coprocessor monitoring  CR0.MP
    mov cr0, rax
    mov rax, cr4
    or eax, (0b11 << 9)     ; Set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
    mov cr4, rax

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
    test r15, FPTEST_SSE2   ; Check if SSE2 test is enabled
    jz FPTests.End          ; Leave tests if disabled

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
    test r15, FPTEST_SSE3   ; Check if SSE3 test is enabled
    jz FPTests.End          ; Leave tests if disabled

    movupd xmm0, [sse3.v1]  ; Load first vector
    movupd xmm1, [sse3.v2]  ; Load second vector
    
    haddpd xmm0, xmm1       ; Horizontally add the values in xmm1 and xmm0, store results in xmm0
  
    movupd [sse3.r], xmm0   ; Write result to memory
    movq rax, xmm0          ; Copy low 64 bits of result to RAX
    lea rsi, [sse3.r]       ; Put address of result into RSI
  
    hlt                     ; Let the host check the result

SSSE3.Test:
    test r15, FPTEST_SSSE3  ; Check if SSSE3 test is enabled
    jz FPTests.End          ; Leave tests if disabled

    movupd xmm0, [ssse3.v]  ; Load vector into xmm0
    movupd xmm1, [ssse3.v]  ; Load vector into xmm1
    
    pabsd xmm0, xmm0        ; Compute absolute values for xmm0
    phaddd xmm1, xmm0       ; Horizontally add each pair of consecutive 32-bit integers from xmm0 and xmm1, pack results into xmm1
    
    movupd [ssse3.r], xmm1  ; Write result to memory
    movq rax, xmm1          ; Copy low 64 bits of result to RAX
    lea rsi, [ssse3.r]      ; Put address of result into RSI
    
    hlt                     ; Let the host check the result

SSE4.Test:   ; Includes SSE4.1 and SSE4.2
    test r15, FPTEST_SSE4   ; Check if SSE4 test is enabled
    jz FPTests.End          ; Leave tests if disabled

    movupd xmm0, [sse4.v1]  ; Load first vector into xmm0
    movupd xmm1, [sse4.v2]  ; Load second vector into xmm1
    movupd xmm2, [sse4.v2]  ; Load third vector into xmm2

    pmuldq xmm0, xmm1       ; [SSE4.1] Multiply first and third signed dwords xmm0
                            ; with first and third signed dwords in xmm1, store qword results in xmm0
    pcmpgtq xmm2, xmm0      ; [SSE4.2] Compare qwords in xmm0 and xmm2 for greater than.
                            ; If true, sets corresponding element in xmm0 to all 1s, otherwise all 0s

    movupd [sse4.r], xmm2   ; Write result to memory
    movq rax, xmm2          ; Copy low 64 bits of result to RAX
    lea rsi, [sse4.r]       ; Put address of result into RSI

    hlt                     ; Let the host check the result

AVX.Enable:
    test r15, FPTEST_AVX    ; Check if AVX test is enabled
    jz FPTests.End          ; Leave tests if disabled

    mov rax, cr4
    or eax, (1 << 18)       ; Set CR4.OSXSAVE
    mov cr4, rax
    xor rcx, rcx
    xgetbv                  ; Load XCR0 register
    or eax, 7               ; Set AVX, SSE, X87 bits
    xsetbv                  ; Save back to XCR0

AVX.Test:
    test r15, FPTEST_AVX    ; Check if AVX test is enabled
    jz FPTests.End          ; Leave tests if disabled

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
    test r15, FPTEST_FMA3   ; Check if FMA3 test is enabled
    jz FPTests.End          ; Leave tests if disabled

    vmovups ymm0, [fma3.v1] ; Load v1 into ymm0
    vmovups ymm1, [fma3.v2] ; Load v2 into ymm1
    vmovups ymm2, [fma3.v3] ; Load v3 into ymm2

    vfmadd132pd ymm0, ymm1, ymm2  ; ymm0 = ymm0 * ymm2 + ymm1

    vmovups [fma3.r], ymm0  ; Write result to memory
    vmovq rax, xmm0         ; Copy low 64 bits of result to RAX
    lea rsi, [fma3.r]       ; Put address of result into RSI

    hlt                     ; Let the host check the result

AVX2.Test:
    test r15, FPTEST_AVX2   ; Check if AVX2 test is enabled
    jz FPTests.End          ; Leave tests if disabled

    vmovups ymm14, [avx2.v] ; Load v into ymm14

    vpermq ymm15, ymm14, 0x1B ; Permutates qwords from ymm6 to reverse order, store result in ymm15

    vmovups [avx2.r], ymm15 ; Write result to memory
    vmovq rax, xmm15        ; Copy low 64 bits of result to RAX
    lea rsi, [avx2.r]       ; Put address of result into RSI

    hlt                     ; Let the host check the result

XSAVE.Test:
    test r15, FPTEST_XSAVE  ; Check if XSAVE test is enabled
    jz FPTests.End          ; Leave tests if disabled

    mov rdx, 0xFFFFFFFFFFFFFFFF  ; Enable all XSAVE features
    mov rax, 0xFFFFFFFFFFFFFFFF  ; ... on RDX and RAX
    xsave [xsavearea]       ; XSAVE to reserved memory area

    mov r8, [xsavearea + 512] ; Read XSTATE_BV
    mov r9, [xsavearea + 520] ; Read XCOMP_BV
    
    mov rdx, 1 << 63
    test r9, rdx
    jz XSAVE.Format.Standard ; Check which format is in use
    mov r11, r9             ; Compacted format in use
    jmp XSAVE.Format.Init
XSAVE.Format.Standard:
    mov r11, r8             ; Standard format in use

XSAVE.Format.Init:
    xor rax, rax

    mov rcx, 16
    lea rdi, [xsavebases]
    rep stosq               ; Clear base addresses

    mov rcx, 16
    lea rdi, [xsavesizes]
    rep stosq               ; Clear sizes

    mov [xsavealign], rax   ; Clear alignment bits

    mov rdx, 1 << 12        ; Initialize RDX with our bit mask
    mov rcx, 11             ; Initialize loop counter
    xor rax, rax

XSAVE.Format.Loop:
    mov r10, rcx            ; Save RCX
    test rdx, r11           ; Check if specified bit is set
    jz XSAVE.Format.Zero    ; Set to zero if clear

    mov rax, 0xD            ; Read CPUID page 0xD for the component...
    add rcx, 1              ; ... RCX + 1 (0 = main, 1 = reserved, 2..62 = correspond to XCR0.n, 63 = reserved)
    cpuid                   ; ... in order to retrieve their base addresses, sizes and alignments
    jmp XSAVE.Format.Write  ; Don't zero out values

XSAVE.Format.Zero:
    xor rbx, rbx            ; Zero out base address
    xor rax, rax            ; Zero out size
    xor rcx, rcx            ; Zero out alignment bit

XSAVE.Format.Write:
    mov [xsavebases + rcx * 8], rbx  ; RBX contains the base address
    mov [xsavesizes + rcx * 8], rax  ; RAX contains the size
    test rcx, 2                      ; RCX contains the alignment bit
    jz XSAVE.Format.Continue ; If the alignment bit is set
    or [xsavealign], rdx    ; ... set the corresponding bit in the destination

XSAVE.Format.Continue:
    mov rcx, r10            ; Restore RCX
    shr rdx, 1              ; Shift test bit
    loop XSAVE.Format.Loop  ; Repeat until all components have been examined

    lea rsi, [xsavearea]    ; Put address of XSAVE area into RSI
    lea r12, [xsavebases]   ; Put address of base addresses into R12
    lea r13, [xsavesizes]   ; Put address of sizes into R13
    lea r14, [xsavealign]   ; Put address of alignment bits into R14
                            ; R8 contains XSTATE_BV
                            ; R9 contains XCOMP_BV

    hlt                     ; Let the host check the result

FPTests.End:
    ; We're done

Die:
    cli
    hlt
    jmp Die
    
    ; Data for MMX test
ALIGN 16
    mmx.v1: dw 5, 10, 15, 20
    mmx.v2: dw 2, 2, 2, 2
    mmx.v3: dw 1, 2, 3, 4
    mmx.r:  resw 4

    ; Data for SSE test
ALIGN 16
    sse.v1: dd 1.1, 2.2, 3.3, 4.4
    sse.v2: dd 5.5, 6.6, 7.7, 8.8
    sse.r:  resd 4

    ; Data for SSE2 test
ALIGN 16
    sse2.v1: dq 1.1, 2.2
    sse2.v2: dq 3.3, 4.4
    sse2.r:  resq 2

    ; Data for SSE3 test
ALIGN 16
    sse3.v1: dq 1.5, 2.5
    sse3.v2: dq 2.5, -0.5
    sse3.r:  resq 2

    ; Data for SSSE3 test
ALIGN 16
    ssse3.v: dd 1234, -4321, -1234, 4321
    ssse3.r: resd 4

    ; Data for SSE4 test
ALIGN 16
    sse4.v1: dd 0, -30, 0, 60
    sse4.v2: dd 0,  -6, 0,  2
    sse4.v3: dq 120, 120
    sse4.r:  resd 4

    ; Data for AVX test
ALIGN 16
    avx.v1: dd 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5
    avx.v2: dd 8.5, 7.5, 6.5, 5.5, 4.5, 3.5, 2.5, 1.5
    avx.v3: dd 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5
    avx.r:  resd 8

    ; Data for FMA3 test
ALIGN 16
    fma3.v1: dq 0.5, 1.0, 1.5, 2.0
    fma3.v2: dq 2.0, 2.5, 3.0, 3.5
    fma3.v3: dq 4.0, 3.0, 2.0, 1.0
    fma3.r:  resq 4

    ; Data for AVX2 test
ALIGN 16
    avx2.v: dq 1, 2, 3, 4
    avx2.r: resq 4

    ; XSAVE state area
ALIGN 4096
    xsavearea: resb 4096        ; XSAVE data area
    xsavebases: times 16 dd 0   ; Base offsets of each XSAVE component
    xsavesizes: times 16 dd 0   ; Sizes of each XSAVE component
    xsavealign: dd 0            ; Alignment bits of each XSAVE component
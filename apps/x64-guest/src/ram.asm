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

    ; Stop for a moment to let the host manipulate the page tables
    hlt

    ; Test new memory allocated by the host
    mov rsi, 0x100000000
    mov rax, [rsi]

    ; Let the host manipulate the page tables again
    hlt

    ; Test modification by the host
    mov rax, [rsi]

    ; Enable SSE and AVX
    mov rax, cr0
    and ax, 0xFFFB      ; Clear coprocessor emulation CR0.EM
    or ax, 0x2          ; Set coprocessor monitoring  CR0.MP
    mov cr0, rax
    mov rax, cr4
    or eax, (0b11 << 9) | (1 << 18)  ; Set CR4.OSFXSR, CR4.OSXMMEXCPT and CR4.OSXSAVE at the same time
    mov cr4, rax
    xor rcx, rcx
    xgetbv              ; Load XCR0 register
    or eax, 7           ; Set AVX, SSE, X87 bits
    xsetbv              ; Save back to XCR0
    
    ; Try some SSE/AVX code
    vzeroall
    vmovups ymm0, [v1]
    vmovups ymm1, [v2]
    vmovups ymm2, [v3]
    vaddps ymm3, ymm0, ymm1
    vmulps ymm3, ymm3, ymm2
    vmovups [v4], ymm3

    ; We're done
Die:
    cli
    hlt
    jmp Die
    
ALIGN 16
    ; Some data for the SSE/AVX code
    v1: dd 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5
    v2: dd 8.5, 7.5, 6.5, 5.5, 4.5, 3.5, 2.5, 1.5
    v3: dd 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5
    v4: dd 0,0,0,0,0,0,0,0

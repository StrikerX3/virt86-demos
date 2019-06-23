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

    ; We're done
    cli
    hlt

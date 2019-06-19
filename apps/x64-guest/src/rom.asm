; Compile with NASM:
;   $ nasm rom.asm -o rom.bin
%define PAGE_PRESENT    (1 << 0)
%define PAGE_WRITE      (1 << 1)
 
%define CODE_SEG     0x0008
%define DATA_SEG     0x0010

; This is where the ROM is loaded
org 0xFFFF0000

; Main code adapted from https://wiki.osdev.org/Entering_Long_Mode_Directly
 
ALIGN 4
IDT:
    .Length       dw 0
    .Base         dd 0
 
; Function to switch directly to long mode from real mode.
; Identity maps the first 2MiB and last 64KiB.
; Uses Intel syntax.
 
; es:edi    Should point to a valid page-aligned 16KiB buffer, for the PML4, PDPT, PD and a PT.
; ss:esp    Should point to memory that can be used as a small (1 uint32_t) stack
 
SwitchToLongMode:
    ; Zero out the 16KiB buffer.
    ; Since we are doing a rep stosd, count should be bytes/4.   
    push di                           ; REP STOSD alters DI.
    mov ecx, 0x1000
    xor eax, eax
    cld
    rep stosd
    pop di                            ; Get DI back.
 
 
    ; Build the Page Map Level 4.
    ; es:di points to the Page Map Level 4 table.
    lea eax, [es:di + 0x1000]         ; Put the address of the Page Directory Pointer Table in to EAX.
    or eax, PAGE_PRESENT | PAGE_WRITE ; Or EAX with the flags - present flag, writable flag.
    mov [es:di], eax                  ; Store the value of EAX as the first PML4E.
 
 
    ; Build the Page Directory Pointer Table.
    lea eax, [es:di + 0x2000]         ; Put the address of the Page Directory in to EAX.
    or eax, PAGE_PRESENT | PAGE_WRITE ; Or EAX with the flags - present flag, writable flag.
    mov [es:di + 0x1000], eax         ; Store the value of EAX as the first PDPTE.
    mov [es:di + 0x1018], eax         ; Store the value of EAX as the 0x18'th PDPTE.
 
    ; Build the Page Directory.
    lea eax, [es:di + 0x3000]         ; Put the address of the Page Table in to EAX.
    or eax, PAGE_PRESENT | PAGE_WRITE ; Or EAX with the flags - present flag, writeable flag.
    mov [es:di + 0x2000], eax         ; Store to value of EAX as the first PDE.

    add eax, 0x1000                   ; Move to the next entry
    mov [es:di + 0x2ff8], eax         ; Store the value of EAX as the last PDE.
 
    push di                           ; Save DI for the time being.

    ; Build the RAM Page Table.
    lea di, [di + 0x3000]             ; Point DI to the RAM page table.
    mov eax, PAGE_PRESENT | PAGE_WRITE    ; Move the flags into EAX - and point it to 0x0000.
 
.LoopRAMPageTable:
    mov [es:di], eax
    add eax, 0x1000
    add di, 8
    cmp eax, 0x200000                 ; If we did all 2MiB, end.
    jb .LoopRAMPageTable

    pop di                            ; Restore DI.
    push di

    ; Build the ROM Page Table.
    lea di, [di + 0x4F80]             ; Point DI to the ROM page table.
    mov eax, 0xFFFF0000 | PAGE_PRESENT | PAGE_WRITE    ; Move the flags into EAX - and point it to 0xFFFF0000.
 
.LoopROMPageTable:
    mov [es:di], eax
    add eax, 0x1000
    add di, 8
    cmp eax, 0xFFFF0000               ; If we did all 64KiB, end.
    jnb .LoopROMPageTable

    pop di                            ; Restore DI.
 
    ; Disable IRQs
    ;mov al, 0xFF                      ; Out 0xFF to 0xA1 and 0x21 to disable all IRQs.
    ;out 0xA1, al
    ;out 0x21, al
 
    nop
    nop
 
    o32 lidt [cs:IDT]                 ; Load a zero length IDT so that any NMI causes a triple fault.
 
    ; Enter long mode.
    mov eax, 10100000b                ; Set the PAE and PGE bit.
    mov cr4, eax
 
    mov edx, edi                      ; Point CR3 at the PML4.
    mov cr3, edx
 
    mov ecx, 0xC0000080               ; Read from the EFER MSR. 
    rdmsr    
 
    or eax, 0x00000100                ; Set the LME bit.
    wrmsr
	; TODO: check why HAXM ignores this
 
    mov ebx, cr0                      ; Activate long mode -
    or ebx, 0x80000001                ; - by enabling paging and protection simultaneously.
    mov cr0, ebx                    
 
    o32 lgdt [cs:GDT.Pointer]         ; Load GDT.Pointer defined below.
 
    jmp dword CODE_SEG:LongMode       ; Load CS with 64 bit segment and flush the instruction cache
 	; TODO: check why HAXM ignores this
 
    ; Global Descriptor Table
ALIGN 16
GDT:
.Null:
    dq 0x0000000000000000             ; Null Descriptor - should be present.
 
.Code:
    dq 0x00209B0000000000             ; 64-bit code descriptor (exec/read).
    dq 0x0000920000000000             ; 64-bit data descriptor (read/write).
 
.End:
ALIGN 4
    dw 0                              ; Padding to make the "address of the GDT" field aligned on a 4-byte boundary
 
.Pointer:
    dw GDT.End - GDT                  ; 16-bit Size (Limit) of GDT.
    dd GDT                            ; 32-bit Base Address of GDT. (CPU will zero extend to 64-bit)
 
 
[BITS 64]      
LongMode:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
 
    ; Jump to program entry point in RAM
    mov rax, 0x10000
    jmp rax


; Reset vector entry point

[BITS 16]
times 0xFFF0 - ($-$$) db 0
    jmp SwitchToLongMode
    cli
    hlt
times 0xFFFF - ($-$$) + 1 hlt
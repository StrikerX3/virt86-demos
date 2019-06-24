/*
Entry point of the basic demo.

This program demonstrates basic usage of the library and performs tests on
the functionality of virtualization platforms.
-------------------------------------------------------------------------------
MIT License

Copyright (c) 2019 Ivan Roberto de Oliveira

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "virt86/virt86.hpp"

#include "print_helpers.hpp"
#include "align_alloc.hpp"
#include "utils.hpp"

#if defined(_WIN32)
#  include <Windows.h>
#elif defined(__linux__)
#  include <stdlib.h>
#elif defined(__APPLE__)
#  include <stdlib.h>
#endif

#include <cstdio>
#include <cinttypes>

using namespace virt86;

// The following flags cause some portions of guest code to be skipped and
// executed on the host by manipulating the virtual processor's registers and
// the guest's physical memory through the hypervisor.
//
// DO_MANUAL_INIT: The GDTR and IDTR are set and the virtual processor is
//   initialized to 32-bit protected mode
// DO_MANUAL_JMP: Performs the jump into 32-bit protected mode.
// DO_MANUAL_PAGING: Sets up the PTEs and the CR3 register for paging.

//#define DO_MANUAL_INIT
//#define DO_MANUAL_JMP
//#define DO_MANUAL_PAGING

int main() {
    // Initialize ROM and RAM
    const uint32_t romSize = PAGE_SIZE * 16;  // 64 KiB
    const uint32_t ramSize = PAGE_SIZE * 256; // 1 MiB
    const uint64_t romBase = 0xFFFF0000;
    const uint64_t ramBase = 0x0;

    uint8_t *rom = alignedAlloc(romSize);
    if (rom == NULL) {
        printf("Failed to allocate memory for ROM\n");
        return -1;
    }
    printf("ROM allocated: %u bytes\n", romSize);

    uint8_t *ram = alignedAlloc(ramSize);
    if (ram == NULL) {
        printf("Failed to allocate memory for RAM\n");
        return -1;
    }
    printf("RAM allocated: %u bytes\n", ramSize);
    printf("\n");

    // Fill ROM with HLT instructions
    memset(rom, 0xf4, romSize);

    // Zero out RAM
    memset(ram, 0, ramSize);

    // Write initialization code to ROM and a simple program to RAM
    {
        uint32_t addr;
#define emit(buf, code) {memcpy(&buf[addr], code, sizeof(code) - 1); addr += sizeof(code) - 1;}

        // --- Start of ROM code ----------------------------------------------------------------------------------------------

        // --- GDT and IDT tables ---------------------------------------------------------------------------------------------

        // GDT table
        addr = 0x0000;
        emit(rom, "\x00\x00\x00\x00\x00\x00\x00\x00"); // [0x0000] GDT entry 0: null
        emit(rom, "\xff\xff\x00\x00\x00\x9b\xcf\x00"); // [0x0008] GDT entry 1: code (full access to 4 GB linear space)
        emit(rom, "\xff\xff\x00\x00\x00\x93\xcf\x00"); // [0x0010] GDT entry 2: data (full access to 4 GB linear space)

        // IDT table (system)
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0018] Vector 0x00: Divide by zero
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0020] Vector 0x01: Reserved
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0028] Vector 0x02: Non-maskable interrupt
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0030] Vector 0x03: Breakpoint (INT3)
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0038] Vector 0x04: Overflow (INTO)
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0040] Vector 0x05: Bounds range exceeded (BOUND)
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0048] Vector 0x06: Invalid opcode (UD2)
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0050] Vector 0x07: Device not available (WAIT/FWAIT)
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0058] Vector 0x08: Double fault
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0060] Vector 0x09: Coprocessor segment overrun
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0068] Vector 0x0A: Invalid TSS
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0070] Vector 0x0B: Segment not present
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0078] Vector 0x0C: Stack-segment fault
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0080] Vector 0x0D: General protection fault
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0088] Vector 0x0E: Page fault
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0090] Vector 0x0F: Reserved
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x0098] Vector 0x10: x87 FPU error
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x00a0] Vector 0x11: Alignment check
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x00a8] Vector 0x12: Machine check
        emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x00b0] Vector 0x13: SIMD Floating-Point Exception
        for (uint8_t i = 0x14; i <= 0x1f; i++) {
            emit(rom, "\x05\x10\x08\x00\x00\x8f\x00\x10"); // [0x00b8..0x0110] Vector 0x14..0x1F: Reserved
        }

        // IDT table (user defined)
        emit(rom, "\x00\x10\x08\x00\x00\x8f\x00\x10"); // [0x0118] Vector 0x20: Just IRET
        emit(rom, "\x02\x10\x08\x00\x00\x8f\x00\x10"); // [0x0120] Vector 0x21: HLT, then IRET

        // --- 32-bit protected mode ------------------------------------------------------------------------------------------

        // Prepare memory for paging
        // (based on https://github.com/unicorn-engine/unicorn/blob/master/tests/unit/test_x86_soft_paging.c)
        // 0x1000 = Page directory
        // 0x2000 = Page table (identity map RAM: 0x000xxxxx)
        // 0x3000 = Page table (identity map ROM: 0xffffxxxx)
        // 0x4000 = Page table (0x10000xxx .. 0x10001xxx -> 0x00005xxx .. 0x00006xxx)
        // 0x5000 = Data area (first dword reads 0xdeadbeef)
        // 0x6000 = Interrupt handler code area
        // 0xe000 = Page table (identity map first page of MMIO: 0xe00000xxx)

        // Load segment registers
        addr = 0xff00;
#ifdef DO_MANUAL_PAGING
        emit(rom, "\xf4");                             // [0xff00] hlt
        emit(rom, "\x90");                             // [0xff01] nop
#else
        emit(rom, "\x33\xc0");                         // [0xff00] xor    eax, eax
#endif
        emit(rom, "\xb0\x10");                         // [0xff02] mov     al, 0x10
        emit(rom, "\x8e\xd8");                         // [0xff04] mov     ds, eax
        emit(rom, "\x8e\xc0");                         // [0xff06] mov     es, eax
        emit(rom, "\x8e\xd0");                         // [0xff08] mov     ss, eax

        // Clear page directory
        emit(rom, "\xbf\x00\x10\x00\x00");             // [0xff0a] mov    edi, 0x1000
        emit(rom, "\xb9\x00\x10\x00\x00");             // [0xff0f] mov    ecx, 0x1000
        emit(rom, "\x31\xc0");                         // [0xff14] xor    eax, eax
        emit(rom, "\xf3\xab");                         // [0xff16] rep    stosd

        // Write 0xdeadbeef at physical memory address 0x5000
        emit(rom, "\xbf\x00\x50\x00\x00");             // [0xff18] mov    edi, 0x5000
        emit(rom, "\xb8\xef\xbe\xad\xde");             // [0xff1d] mov    eax, 0xdeadbeef
        emit(rom, "\x89\x07");                         // [0xff22] mov    [edi], eax

        // Identity map the RAM to 0x00000000
        emit(rom, "\xb9\x00\x01\x00\x00");             // [0xff24] mov    ecx, 0x100
        emit(rom, "\xbf\x00\x20\x00\x00");             // [0xff29] mov    edi, 0x2000
        emit(rom, "\xb8\x03\x00\x00\x00");             // [0xff2e] mov    eax, 0x0003
        //                                             // aLoop:
        emit(rom, "\xab");                             // [0xff33] stosd
        emit(rom, "\x05\x00\x10\x00\x00");             // [0xff34] add    eax, 0x1000
        emit(rom, "\xe2\xf8");                         // [0xff39] loop   aLoop

        // Identity map the ROM
        emit(rom, "\xb9\x10\x00\x00\x00");             // [0xff3b] mov    ecx, 0x10
        emit(rom, "\xbf\xc0\x3f\x00\x00");             // [0xff40] mov    edi, 0x3fc0
        emit(rom, "\xb8\x03\x00\xff\xff");             // [0xff45] mov    eax, 0xffff0003
        //                                             // bLoop:
        emit(rom, "\xab");                             // [0xff4a] stosd
        emit(rom, "\x05\x00\x10\x00\x00");             // [0xff4b] add    eax, 0x1000
        emit(rom, "\xe2\xf8");                         // [0xff50] loop   bLoop

        // Map physical address 0x5000 to virtual address 0x10000000
        emit(rom, "\xbf\x00\x40\x00\x00");             // [0xff52] mov    edi, 0x4000
        emit(rom, "\xb8\x03\x50\x00\x00");             // [0xff57] mov    eax, 0x5003
        emit(rom, "\x89\x07");                         // [0xff5c] mov    [edi], eax

        // Map physical address 0x6000 to virtual address 0x10001000
        emit(rom, "\xbf\x04\x40\x00\x00");             // [0xff5e] mov    edi, 0x4004
        emit(rom, "\xb8\x03\x60\x00\x00");             // [0xff63] mov    eax, 0x6003
        emit(rom, "\x89\x07");                         // [0xff68] mov    [edi], eax

        // Map physical address 0xe0000000 to virtual address 0xe0000000 (for MMIO)
        emit(rom, "\xbf\x00\xe0\x00\x00");             // [0xff6a] mov    edi, 0xe000
        emit(rom, "\xb8\x03\x00\x00\xe0");             // [0xff6f] mov    eax, 0xe0000003
        emit(rom, "\x89\x07");                         // [0xff74] mov    [edi], eax

        // Add page tables into page directory
        emit(rom, "\xbf\x00\x10\x00\x00");             // [0xff76] mov    edi, 0x1000
        emit(rom, "\xb8\x03\x20\x00\x00");             // [0xff7b] mov    eax, 0x2003
        emit(rom, "\x89\x07");                         // [0xff80] mov    [edi], eax
        emit(rom, "\xbf\xfc\x1f\x00\x00");             // [0xff82] mov    edi, 0x1ffc
        emit(rom, "\xb8\x03\x30\x00\x00");             // [0xff87] mov    eax, 0x3003
        emit(rom, "\x89\x07");                         // [0xff8c] mov    [edi], eax
        emit(rom, "\xbf\x00\x11\x00\x00");             // [0xff8e] mov    edi, 0x1100
        emit(rom, "\xb8\x03\x40\x00\x00");             // [0xff93] mov    eax, 0x4003
        emit(rom, "\x89\x07");                         // [0xff98] mov    [edi], eax
        emit(rom, "\xbf\x00\x1e\x00\x00");             // [0xff9a] mov    edi, 0x1e00
        emit(rom, "\xb8\x03\xe0\x00\x00");             // [0xff9f] mov    eax, 0xe003
        emit(rom, "\x89\x07");                         // [0xffa4] mov    [edi], eax

        // Load the page directory register
        emit(rom, "\xb8\x00\x10\x00\x00");             // [0xffa6] mov    eax, 0x1000
        emit(rom, "\x0f\x22\xd8");                     // [0xffab] mov    cr3, eax

        // Enable paging
        emit(rom, "\x0f\x20\xc0");                     // [0xffae] mov    eax, cr0
        emit(rom, "\x0d\x00\x00\x00\x80");             // [0xffb1] or     eax, 0x80000000
        emit(rom, "\x0f\x22\xc0");                     // [0xffb6] mov    cr0, eax

        // Clear EAX
        emit(rom, "\x31\xc0");                         // [0xffb9] xor    eax, eax

        // Load using virtual memory address; EAX = 0xdeadbeef
        emit(rom, "\xbe\x00\x00\x00\x10");             // [0xffbb] mov    esi, 0x10000000
        emit(rom, "\x8b\x06");                         // [0xffc0] mov    eax, [esi]

        // First stop
        emit(rom, "\xf4");                             // [0xffc2] hlt

        // Jump to RAM
        emit(rom, "\xe9\x3c\x00\x00\x10");             // [0xffc3] jmp    0x10000004
        // .. ends at 0xffc7

        // --- 16-bit real mode transition to 32-bit protected mode -----------------------------------------------------------

        // Load GDT and IDT tables
        addr = 0xffd0;
        emit(rom, "\x66\x2e\x0f\x01\x16\xf2\xff");     // [0xffd0] lgdt   [cs:0xfff2]
        emit(rom, "\x66\x2e\x0f\x01\x1e\xf8\xff");     // [0xffd7] lidt   [cs:0xfff8]

        // Enter protected mode
        emit(rom, "\x0f\x20\xc0");                     // [0xffde] mov    eax, cr0
        emit(rom, "\x0c\x01");                         // [0xffe1] or      al, 1
        emit(rom, "\x0f\x22\xc0");                     // [0xffe3] mov    cr0, eax
#ifdef DO_MANUAL_JMP
        emit(rom, "\xf4")                              // [0xffe6] hlt
        // Fill the rest with HLTs
        while (addr < 0xfff0) {
            emit(rom, "\xf4");                         // [0xffe7..0xffef] hlt
        }
#else
        emit(rom, "\x66\xea\x00\xff\xff\xff\x08\x00"); // [0xffe6] jmp    dword 0x8:0xffffff00
        emit(rom, "\xf4");                             // [0xffef] hlt
#endif

        // --- 16-bit real mode start -----------------------------------------------------------------------------------------

        // Jump to initialization code and define GDT/IDT table pointer
        addr = 0xfff0;
#ifdef DO_MANUAL_INIT
        emit(rom, "\xf4");                             // [0xfff0] hlt
        emit(rom, "\x90");                             // [0xfff1] nop
#else
        emit(rom, "\xeb\xde");                         // [0xfff0] jmp    short 0x1d0
#endif
        emit(rom, "\x18\x00\x00\x00\xff\xff");         // [0xfff2] GDT pointer: 0xffff0000:0x0018
        emit(rom, "\x10\x01\x18\x00\xff\xff");         // [0xfff8] IDT pointer: 0xffff0018:0x0110
        
        // There's room for two bytes at the end, so let's fill it up with HLTs
        emit(rom, "\xf4");                             // [0xfffe] hlt
        emit(rom, "\xf4");                             // [0xffff] hlt

        // --- End of ROM code ------------------------------------------------------------------------------------------------

        // --- Start of RAM code ----------------------------------------------------------------------------------------------
        addr = 0x5004; // Addresses 0x5000..0x5003 are reserved for 0xdeadbeef
        // Note that these addresses are mapped to virtual addresses 0x10000000 through 0x10000fff

        // Do some basic stuff
        emit(ram, "\xba\x78\x56\x34\x12");             // [0x5004] mov    edx, 0x12345678
        emit(ram, "\xbf\x00\x00\x00\x10");             // [0x5009] mov    edi, 0x10000000
        emit(ram, "\x31\xd0");                         // [0x500e] xor    eax, edx
        emit(ram, "\x89\x07");                         // [0x5010] mov    [edi], eax
        emit(ram, "\xf4");                             // [0x5012] hlt

        // Setup a proper stack
        emit(ram, "\x31\xed");                         // [0x5013] xor    ebp, ebp
        emit(ram, "\xbc\x00\x00\x10\x00");             // [0x5015] mov    esp, 0x100000

        // Test the stack
        emit(ram, "\x68\xfe\xca\x0d\xf0");             // [0x501a] push   0xf00dcafe
        emit(ram, "\x5a");                             // [0x501f] pop    edx
        emit(ram, "\xf4");                             // [0x5020] hlt

        // -------------------------------

        // Call interrupts
        emit(ram, "\xcd\x20");                         // [0x5021] int    0x20
        emit(ram, "\xcd\x21");                         // [0x5023] int    0x21
        emit(ram, "\xf4");                             // [0x5025] hlt

        // -------------------------------

        // Basic PMIO
        emit(ram, "\x66\xba\x00\x10");                 // [0x5026] mov     dx, 0x1000
        emit(ram, "\xec");                             // [0x502a] in      al, dx
        emit(ram, "\x66\x42");                         // [0x502b] inc     dx
        emit(ram, "\x34\xff");                         // [0x502d] xor     al, 0xff
        emit(ram, "\xee");                             // [0x502f] out     dx, al
        emit(ram, "\x66\x42");                         // [0x5030] inc     dx
        emit(ram, "\x66\xed");                         // [0x5032] in      ax, dx
        emit(ram, "\x66\x42");                         // [0x5034] inc     dx
        emit(ram, "\x66\x83\xf0\xff");                 // [0x5036] xor     ax, 0xffff
        emit(ram, "\x66\xef");                         // [0x503a] out     dx, ax
        emit(ram, "\x66\x42");                         // [0x503c] inc     dx
        emit(ram, "\xed");                             // [0x503e] in     eax, dx
        emit(ram, "\x66\x42");                         // [0x503f] inc     dx
        emit(ram, "\x83\xf0\xff");                     // [0x5041] xor    eax, 0xffffffff
        emit(ram, "\xef");                             // [0x5044] out     dx, eax

        // -------------------------------

        // Basic MMIO
        emit(ram, "\xbf\x00\x00\x00\xe0");             // [0x5045] mov    edi, 0xe0000000
        emit(ram, "\x8b\x1f");                         // [0x504a] mov    ebx, [edi]
        emit(ram, "\x83\xc7\x04");                     // [0x504c] add    edi, 4
        emit(ram, "\x89\x1f");                         // [0x504f] mov    [edi], ebx

        // Advanced MMIO
        emit(ram, "\xb9\x00\x00\x00\x10");             // [0x5051] mov    ecx, 0x10000000
        emit(ram, "\x85\x0f");                         // [0x5056] test   [edi], ecx

        // -------------------------------

        // Test single stepping
        emit(ram, "\xb9\x11\x00\x00\x00");             // [0x5058] mov    ecx, 0x11
        emit(ram, "\xb9\x00\x22\x00\x00");             // [0x505d] mov    ecx, 0x2200
        emit(ram, "\xb9\x00\x00\x33\x00");             // [0x5062] mov    ecx, 0x330000
        emit(ram, "\xb9\x00\x00\x00\x44");             // [0x5067] mov    ecx, 0x44000000

        // -------------------------------

        // Test software and hardware breakpoints
        emit(ram, "\xb9\xff\x00\x00\x00");             // [0x506c] mov    ecx, 0xff
        emit(ram, "\xb9\x00\xee\x00\x00");             // [0x5071] mov    ecx, 0xee00
        emit(ram, "\xb9\x00\x00\xdd\x00");             // [0x5076] mov    ecx, 0xdd0000
        emit(ram, "\xb9\x00\x00\x00\xcc");             // [0x507b] mov    ecx, 0xcc000000
        emit(ram, "\xb9\xff\xee\xdd\xcc");             // [0x5080] mov    ecx, 0xccddeeff

        // -------------------------------

        // Test CPUID exit
        emit(ram, "\x33\xc0");                         // [0x5085] xor    eax, eax
        emit(ram, "\x0f\xa2");                         // [0x5087] cpuid

        // Test custom CPUID
        emit(ram, "\xb8\x02\x00\x00\x80");             // [0x5089] xor    eax, eax
        emit(ram, "\x0f\xa2");                         // [0x508e] cpuid
        emit(ram, "\xf4");                             // [0x5090] hlt

        // -------------------------------

        // End
        emit(ram, "\xf4");                             // [0x5091] hlt

        // -------------------------------

        addr = 0x6000; // Interrupt handlers
        // Note that these addresses are mapped to virtual addresses 0x10001000 through 0x10001fff
        // 0x20: Just IRET
        emit(ram, "\xfb");                             // [0x6000] sti
        emit(ram, "\xcf");                             // [0x6001] iretd

        // 0x21: HLT, then IRET
        emit(ram, "\xf4");                             // [0x6002] hlt
        emit(ram, "\xfb");                             // [0x6003] sti
        emit(ram, "\xcf");                             // [0x6004] iretd

        // 0x00 .. 0x1F: Clear stack then IRET
        emit(ram, "\x83\xc4\x04");                     // [0x6005] add    esp, 4
        emit(ram, "\xfb");                             // [0x6008] sti
        emit(ram, "\xcf");                             // [0x6009] iretd

#undef emit
    }

    // ----- Hypervisor platform initialization -------------------------------------------------------------------------------

    // Pick the first hypervisor platform that is available and properly initialized on this system.
    printf("Loading virtualization platforms... ");

    bool foundPlatform = false;
    size_t platformIndex = 0;
    for (size_t i = 0; i < array_size(PlatformFactories); i++) {
        const Platform& platform = PlatformFactories[i]();
        if (platform.GetInitStatus() == PlatformInitStatus::OK) {
            printf("%s loaded successfully\n", platform.GetName().c_str());
            foundPlatform = true;
            platformIndex = i;
            break;
        }
    }

    if (!foundPlatform) {
        printf("none found\n");
        return -1;
    }

    Platform& platform = PlatformFactories[platformIndex]();

    // Print out the platform's features
    printf("Features:\n");
    auto& features = platform.GetFeatures();
    printf("  Maximum number of VCPUs: %u per VM, %u global\n", features.maxProcessorsPerVM, features.maxProcessorsGlobal);
	printf("  Maximum guest physical address: 0x%llx\n", HostInfo.gpa.maxAddress);
	printf("  Unrestricted guest: %s\n", (features.unrestrictedGuest) ? "supported" : "unsuported");
    printf("  Extended Page Tables: %s\n", (features.extendedPageTables) ? "supported" : "unsuported");
    printf("  Guest debugging: %s\n", (features.guestDebugging) ? "available" : "unavailable");
    printf("  Memory protection: %s\n", (features.guestMemoryProtection) ? "available" : "unavailable");
    printf("  Dirty page tracking: %s\n", (features.dirtyPageTracking) ? "available" : "unavailable");
    printf("  Partial dirty bitmap querying: %s\n", (features.partialDirtyBitmap) ? "supported" : "unsupported");
    printf("  Large memory allocation: %s\n", (features.largeMemoryAllocation) ? "supported" : "unsuported");
    printf("  Memory aliasing: %s\n", (features.memoryAliasing) ? "supported" : "unsuported");
    printf("  Memory unmapping: %s\n", (features.memoryUnmapping) ? "supported" : "unsuported");
    printf("  Partial unmapping: %s\n", (features.partialUnmapping) ? "supported" : "unsuported");
    printf("  Partial MMIO instructions: %s\n", (features.partialMMIOInstructions) ? "yes" : "no");
    printf("  Custom CPUID results: %s\n", (features.customCPUIDs) ? "supported" : "unsupported");
    if (features.customCPUIDs && features.supportedCustomCPUIDs.size() > 0) {
        printf("       Function        EAX         EBX         ECX         EDX\n");
        for (auto it = features.supportedCustomCPUIDs.cbegin(); it != features.supportedCustomCPUIDs.cend(); it++) {
            printf("      0x%08x = 0x%08x  0x%08x  0x%08x  0x%08x\n", it->function, it->eax, it->ebx, it->ecx, it->edx);
        }
    }
    printf("  Floating point extensions:");
    const auto fpExts = BitmaskEnum(features.floatingPointExtensions);
    if (!fpExts) printf(" None");
    else {
        if (fpExts.AnyOf(FloatingPointExtension::SSE2)) printf(" SSE2");
        if (fpExts.AnyOf(FloatingPointExtension::AVX)) printf(" AVX");
        if (fpExts.AnyOf(FloatingPointExtension::VEX)) printf(" VEX");
        if (fpExts.AnyOf(FloatingPointExtension::MVEX)) printf(" MVEX");
        if (fpExts.AnyOf(FloatingPointExtension::EVEX)) printf(" EVEX");
    }
    printf("\n");
    printf("  Extended control registers:");
    const auto extCRs = BitmaskEnum(features.extendedControlRegisters);
    if (!extCRs) printf(" None");
    else {
        if (extCRs.AnyOf(ExtendedControlRegister::CR8)) printf(" CR8");
        if (extCRs.AnyOf(ExtendedControlRegister::XCR0)) printf(" XCR0");
        if (extCRs.AnyOf(ExtendedControlRegister::MXCSRMask)) printf(" MXCSR_MASK");
    }
    printf("\n");
    printf("  Extended VM exits:");
    const auto extVMExits = BitmaskEnum(features.extendedVMExits);
    if (!extVMExits) printf(" None");
    else {
        if (extVMExits.AnyOf(ExtendedVMExit::CPUID)) printf(" CPUID");
        if (extVMExits.AnyOf(ExtendedVMExit::MSRAccess)) printf(" MSRAccess");
        if (extVMExits.AnyOf(ExtendedVMExit::Exception)) printf(" Exception");
    }
    printf("\n");
    printf("  Exception exits:");
    const auto excptExits = BitmaskEnum(features.exceptionExits);
    if (!excptExits) printf(" None");
    else {
        if (excptExits.AnyOf(ExceptionCode::DivideErrorFault)) printf(" DivideErrorFault");
        if (excptExits.AnyOf(ExceptionCode::DebugTrapOrFault)) printf(" DebugTrapOrFault");
        if (excptExits.AnyOf(ExceptionCode::BreakpointTrap)) printf(" BreakpointTrap");
        if (excptExits.AnyOf(ExceptionCode::OverflowTrap)) printf(" OverflowTrap");
        if (excptExits.AnyOf(ExceptionCode::BoundRangeFault)) printf(" BoundRangeFault");
        if (excptExits.AnyOf(ExceptionCode::InvalidOpcodeFault)) printf(" InvalidOpcodeFault");
        if (excptExits.AnyOf(ExceptionCode::DeviceNotAvailableFault)) printf(" DeviceNotAvailableFault");
        if (excptExits.AnyOf(ExceptionCode::DoubleFaultAbort)) printf(" DoubleFaultAbort");
        if (excptExits.AnyOf(ExceptionCode::InvalidTaskStateSegmentFault)) printf(" InvalidTaskStateSegmentFault");
        if (excptExits.AnyOf(ExceptionCode::SegmentNotPresentFault)) printf(" SegmentNotPresentFault");
        if (excptExits.AnyOf(ExceptionCode::StackFault)) printf(" StackFault");
        if (excptExits.AnyOf(ExceptionCode::GeneralProtectionFault)) printf(" GeneralProtectionFault");
        if (excptExits.AnyOf(ExceptionCode::PageFault)) printf(" PageFault");
        if (excptExits.AnyOf(ExceptionCode::FloatingPointErrorFault)) printf(" FloatingPointErrorFault");
        if (excptExits.AnyOf(ExceptionCode::AlignmentCheckFault)) printf(" AlignmentCheckFault");
        if (excptExits.AnyOf(ExceptionCode::MachineCheckAbort)) printf(" MachineCheckAbort");
        if (excptExits.AnyOf(ExceptionCode::SimdFloatingPointFault)) printf(" SimdFloatingPointFault");
    }
    printf("\n\n");

    // Create virtual machine
    VMSpecifications vmSpecs = { 0 };
    vmSpecs.numProcessors = 1;
    vmSpecs.extendedVMExits = ExtendedVMExit::CPUID;
    vmSpecs.vmExitCPUIDFunctions.push_back(0);
    vmSpecs.CPUIDResults.emplace_back(0x80000002, 'vupc', ' tri', 'UPCV', '    ');
    printf("Creating virtual machine... ");
    auto opt_vm = platform.CreateVM(vmSpecs);
    if (!opt_vm) {
        printf("failed\n");
        return -1;
    }
    printf("succeeded\n");
    VirtualMachine& vm = opt_vm->get();
    
    // Map ROM to the top of the 32-bit address range
    printf("Mapping ROM... ");
    auto memMapStatus = vm.MapGuestMemory(romBase, romSize, MemoryFlags::Read | MemoryFlags::Execute, rom);
    switch (memMapStatus) {
    case MemoryMappingStatus::OK: printf("succeeded\n"); break;
    case MemoryMappingStatus::Unsupported: printf("failed: unsupported operation\n"); return -1;
    case MemoryMappingStatus::MisalignedHostMemory: printf("failed: memory host block is misaligned\n"); return -1;
    case MemoryMappingStatus::MisalignedAddress: printf("failed: base address is misaligned\n"); return -1;
    case MemoryMappingStatus::MisalignedSize: printf("failed: size is misaligned\n"); return -1;
    case MemoryMappingStatus::EmptyRange: printf("failed: size is zero\n"); return -1;
    case MemoryMappingStatus::AlreadyAllocated: printf("failed: host memory block is already allocated\n"); return -1;
    case MemoryMappingStatus::InvalidFlags: printf("failed: invalid flags supplied\n"); return -1;
    case MemoryMappingStatus::Failed: printf("failed\n"); return -1;
	case MemoryMappingStatus::OutOfBounds: printf("out of bounds\n"); return -1;
	default: printf("failed: unhandled reason (%d)\n", static_cast<int>(memMapStatus)); return -1;
    }

    // Alias ROM to the top of the 31-bit address range if supported
    // TODO: test memory aliasing in the virtual machine
    if (features.memoryAliasing) {
        printf("Mapping ROM alias... ");
        memMapStatus = vm.MapGuestMemory(romBase >> 1, romSize, MemoryFlags::Read | MemoryFlags::Execute, rom);
        switch (memMapStatus) {
        case MemoryMappingStatus::OK: printf("succeeded\n"); break;
        case MemoryMappingStatus::Unsupported: printf("failed: unsupported operation\n"); return -1;
        case MemoryMappingStatus::MisalignedHostMemory: printf("failed: memory host block is misaligned\n"); return -1;
        case MemoryMappingStatus::MisalignedAddress: printf("failed: base address is misaligned\n"); return -1;
        case MemoryMappingStatus::MisalignedSize: printf("failed: size is misaligned\n"); return -1;
        case MemoryMappingStatus::EmptyRange: printf("failed: size is zero\n"); return -1;
        case MemoryMappingStatus::AlreadyAllocated: printf("failed: host memory block is already allocated\n"); return -1;
        case MemoryMappingStatus::InvalidFlags: printf("failed: invalid flags supplied\n"); return -1;
        case MemoryMappingStatus::Failed: printf("failed\n"); return -1;
		case MemoryMappingStatus::OutOfBounds: printf("out of bounds\n"); return -1;
		default: printf("failed: unhandled reason (%d)\n", static_cast<int>(memMapStatus)); return -1;
        }
    }

    // Map RAM to the bottom of the 32-bit address range
    printf("Mapping RAM... ");
    memMapStatus = vm.MapGuestMemory(ramBase, ramSize, MemoryFlags::Read | MemoryFlags::Write | MemoryFlags::Execute | MemoryFlags::DirtyPageTracking, ram);
    switch (memMapStatus) {
    case MemoryMappingStatus::OK: printf("succeeded\n"); break;
    case MemoryMappingStatus::Unsupported: printf("failed: unsupported operation\n"); return -1;
    case MemoryMappingStatus::MisalignedHostMemory: printf("failed: memory host block is misaligned\n"); return -1;
    case MemoryMappingStatus::MisalignedAddress: printf("failed: base address is misaligned\n"); return -1;
    case MemoryMappingStatus::MisalignedSize: printf("failed: size is misaligned\n"); return -1;
    case MemoryMappingStatus::EmptyRange: printf("failed: size is zero\n"); return -1;
    case MemoryMappingStatus::AlreadyAllocated: printf("failed: host memory block is already allocated\n"); return -1;
    case MemoryMappingStatus::InvalidFlags: printf("failed: invalid flags supplied\n"); return -1;
    case MemoryMappingStatus::Failed: printf("failed\n"); return -1;
	case MemoryMappingStatus::OutOfBounds: printf("out of bounds\n"); return -1;
	default: printf("failed: unhandled reason (%d)\n", static_cast<int>(memMapStatus)); return -1;
    }

    // Get the virtual processor
    printf("Retrieving virtual processor... ");
    auto opt_vp = vm.GetVirtualProcessor(0);
    if (!opt_vp) {
        printf("failed\n");
        return -1;
    }
    auto& vp = opt_vp->get();
    printf("succeeded\n");

    printf("\nInitial CPU register state:\n");
    printRegs(vp);
    printf("\n");

    VPOperationStatus opStatus;

#ifdef DO_MANUAL_INIT
    {
        RegValue cr0, eip;

        vp.RegRead(Reg::CR0, cr0);
        vp.RegRead(Reg::EIP, eip);

        // Load GDT table
        RegValue gdtr;
        gdtr.table.base = romBase;
        gdtr.table.limit = 0x0018;

        // Load IDT table
        RegValue idtr;
        idtr.table.base = romBase + 0x18;
        idtr.table.limit = 0x0110;

        // Enter protected mode
        cr0.u32 |= CR0_PE;

        // Skip initialization code
        eip.u32 = 0xffe6;

        vp.RegWrite(Reg::GDTR, gdtr);
        vp.RegWrite(Reg::IDTR, idtr);

        vp.RegWrite(Reg::CR0, cr0);
        vp.RegWrite(Reg::EIP, eip);
    }
#endif

    // ----- Start of emulation -----------------------------------------------------------------------------------------------

    printf("Starting tests!\n");

    // The CPU starts in 16-bit real mode.
    // Memory addressing is based on segments and offsets, where a segment is basically a 16-byte offset.

    // On a real application, you should be checking the outcome of register reads and writes.
    // We're not going to bother since we know they cannot fail, except for segment registers.

    // Run the CPU! Will stop at the first HLT at ROM address 0xffc2
    auto execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    printf("\nCPU register state after 16-bit initialization code:\n");
    printRegs(vp);
    printf("\n");

    if (features.partialDirtyBitmap) {
        printDirtyBitmap(vm, 0x0, 0x10);
    }
    else {
        printf("Hypervisor does not support reading partial dirty bitmaps\n\n");
        printDirtyBitmap(vm, 0x0, (ramSize + PAGE_SIZE - 1) / PAGE_SIZE);
    }

#ifdef DO_MANUAL_JMP
    {
        // Do the jmp dword 0x8:0xffffff00 manually
        RegValue cs;
        if (vp.ReadSegment(0x0008, cs) != VPOperationStatus::OK) {
            printf("Failed to load segment data for selector 0x0008\n");
            return -1;
        }
        opStatus = vp.RegWrite(Reg::CS, cs);
        if (opStatus != VPOperationStatus::OK) {
            printf("Failed to set CS register\n");
            return -1;
        }
        vp.RegWrite(Reg::EIP, 0xffffff00);
    }

    // Run the CPU again!
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    printf("\nCPU register state after manual jump:\n");
    printRegs(vp);
    printf("\n");

#endif

#ifdef DO_MANUAL_PAGING
    {
        // Prepare the registers
        Reg regs[] = {
            Reg::EAX, Reg::ESI, Reg::EIP, Reg::CR0, Reg::CR3,
            Reg::SS, Reg::DS, Reg::ES,
        };
        RegValue values[] = {
            0, 0x10000000, 0xffffffc0, 0xe0000011, 0x1000,
            0x0010, 0x0010, 0x0010,
        };

        for (int i = 5; i < 8; i++) {
            if (vp.ReadSegment(0x0010, values[i]) != VPOperationStatus::OK) {
                printf("Failed to load segment data for selector 0x0010\n");
                return -1;
            }
        }

        opStatus = vp.RegWrite(regs, values, array_size(regs));
        if (opStatus != VPOperationStatus::OK) {
            printf("Failed to set VCPU registers\n");
            return -1;
        }

        // Clear page directory
        memset(&ram[0x1000], 0, 0x1000 * sizeof(uint16_t));

        // Write 0xdeadbeef at physical memory address 0x5000
        *(uint32_t *)&ram[0x5000] = 0xdeadbeef;

        // Identity map the RAM to 0x00000000
        for (uint32_t i = 0; i < 0x100; i++) {
            *(uint32_t *)&ram[0x2000 + i * 4] = 0x0003 + i * 0x1000;
        }

        // Identity map the ROM
        for (uint32_t i = 0; i < 0x10; i++) {
            *(uint32_t *)&ram[0x3fc0 + i * 4] = 0xffff0003 + i * 0x1000;
        }

        // Map physical address 0x5000 to virtual address 0x10000000
        *(uint32_t *)&ram[0x4000] = 0x5003;

        // Map physical address 0x6000 to virtual address 0x10001000
        *(uint32_t *)&ram[0x4004] = 0x6003;

        // Map physical address 0xe0000000 to virtual address 0xe0000000
        *(uint32_t *)&ram[0xe000] = 0xe0000003;

        // Add page tables into page directory
        *(uint32_t *)&ram[0x1000] = 0x2003;
        *(uint32_t *)&ram[0x1ffc] = 0x3003;
        *(uint32_t *)&ram[0x1100] = 0x4003;
        *(uint32_t *)&ram[0x1e00] = 0xe003;

        // Run the CPU again!
        execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to run\n");
            return -1;
        }
    }

    printf("\nCPU register state after manual paging setup:\n");
    printRegs(vp);
    printf("\n");

#endif

    // ----- Access data in virtual memory ------------------------------------------------------------------------------------

    printf("Testing data in virtual memory\n\n");

    // Validate output at the first stop
    auto& exitInfo = vp.GetVMExitInfo();
    {
        // Get CPU registers
        RegValue cs, eip, eax;

        opStatus = vp.RegRead(Reg::CS, cs);
        if (opStatus != VPOperationStatus::OK) {
            printf("Failed to read CS register\n");
            return -1;
        }
        vp.RegRead(Reg::EIP, eip);
        vp.RegRead(Reg::EAX, eax);

        // Validate
        if (eip.u32 == 0xffffffc3 && cs.u16 == 0x0008) {
            printf("Emulation stopped at the right place!\n");
            if (eax.u32 == 0xdeadbeef) {
                printf("And we got the right result!\n");
            }
        }
    }

    printf("\n");

    // ----- Execute code in virtual memory -----------------------------------------------------------------------------------

    printf("Testing code in virtual memory\n\n");

    // Run CPU
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::HLT:
        printf("Emulation exited due to HLT instruction as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    // Validate output
    {
        // Get CPU registers
        RegValue eip, eax, edx;

        vp.RegRead(Reg::EIP, eip);
        vp.RegRead(Reg::EAX, eax);
        vp.RegRead(Reg::EDX, edx);

        if (eip.u32 == 0x10000013) {
            printf("Emulation stopped at the right place!\n");
            const uint32_t memValue = *(uint32_t *)&ram[0x5000];
            if (eax.u32 == 0xcc99e897 && edx.u32 == 0x12345678 && memValue == 0xcc99e897) {
                printf("And we got the right result!\n");
            }
        }
    }
    
    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");

    // ----- Stack ------------------------------------------------------------------------------------------------------------

    printf("Testing the stack\n\n");

    // Run CPU
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::HLT:
        printf("Emulation exited due to HLT instruction as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    // Validate stack results
    {
        RegValue eip, edx, esp;
        vp.RegRead(Reg::EIP, eip);
        vp.RegRead(Reg::EDX, edx);
        vp.RegRead(Reg::ESP, esp);

        if (eip.u32 == 0x10000021) {
            printf("Emulation stopped at the right place!\n");
            const uint32_t memValue = *(uint32_t *)&ram[0xffffc];
            if (edx.u32 == 0xf00dcafe && esp.u32 == 0x00100000 && memValue == 0xf00dcafe) {
                printf("And we got the right result!\n");
            }
        }
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");

    // ----- Interrupts -------------------------------------------------------------------------------------------------------

    printf("Testing interrupts\n\n");

    // Run until the HLT inside INT 0x21
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::HLT:
        printf("Emulation exited due to HLT instruction as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    // Validate registers
    {
        RegValue eip;
        vp.RegRead(Reg::EIP, eip);

        if (eip.u32 == 0x10001003) {
            printf("Emulation stopped at the right place!\n");
        }
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");


    // Now we should leave the interrupt handler and hit the HLT right after INT 0x21
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::HLT:
        printf("Emulation exited due to HLT instruction as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    // Validate registers
    {
        RegValue eip;
        vp.RegRead(Reg::EIP, eip);
        
        if (eip.u32 == 0x10000026) {
            printf("Emulation stopped at the right place!\n");
        }
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");


    // Enable interrupts
    {
        RegValue eflags;
        vp.RegRead(Reg::EFLAGS, eflags);
        eflags.u32 |= RFLAGS_IF;
        vp.RegWrite(Reg::EFLAGS, eflags);
    }

    // Inject an INT 0x21
    vp.EnqueueInterrupt(0x21);

    // Should hit the HLT in the INT 0x21 handler again
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    // Some hypervisors cause a VM exit due to either having to cancel
    // execution of the virtual processor to open a window for interrupt
    // injection, or because of the act of requesting an injection window.
    if (exitInfo.reason == VMExitReason::Cancelled || exitInfo.reason == VMExitReason::Interrupt) {
        printf("Emulation exited to inject an interrupt, continuing execution...\n");
        execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to run\n");
            return -1;
        }
    }

    switch (exitInfo.reason) {
    case VMExitReason::HLT:
        printf("Emulation exited due to HLT instruction as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    // Validate registers
    {
        RegValue eip;
        vp.RegRead(Reg::EIP, eip);

        if (eip.u32 == 0x10001003) {
            printf("Emulation stopped at the right place!\n");
        }
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");

    // ----- I/O and MMIO test preparation ------------------------------------------------------------------------------------

    // NOTE: typically in a program you'd register your own I/O callbacks once,
    // but for the purposes of readability we're going to change the callbacks on every test.

    // Define lambdas for unexpected callbacks
    const auto unexpectedIORead = [](void *, uint16_t port, size_t size) noexcept -> uint32_t {
        printf("** Unexpected I/O read from port 0x%x (%zd bytes)\n", port, size);
        return 0;
    };
    const auto unexpectedIOWrite = [](void *, uint16_t port, size_t size, uint32_t value) noexcept {
        printf("** Unexpected I/O write to port 0x%x (%zd bytes) = 0x%x\n", port, size, value);
    };
    const auto unexpectedMMIORead = [](void *, uint64_t address, size_t size) noexcept -> uint64_t {
        printf("** Unexpected MMIO read from address 0x%" PRIx64 " (%zd bytes)\n", address, size);
        return 0;
    };
    const auto unexpectedMMIOWrite = [](void *, uint64_t address, size_t size, uint64_t value) noexcept {
        printf("** Unexpected MMIO write to address 0x%" PRIx64 " (%zd bytes)\n = 0x%" PRIx64 "", address, size, value);
    };

    // ----- PIO --------------------------------------------------------------------------------------------------------------

    printf("Testing PIO\n\n");

    // Setup I/O callbacks
    vm.RegisterIOReadCallback([](void *, uint16_t port, size_t size) noexcept -> uint32_t {
        printf("I/O read callback reached!\n");
        if (port == 0x1000 && size == 1) {
            printf("And we got the right port and size!\n");
            return 0xac;
        }
        return 0;
    });
    vm.RegisterIOWriteCallback(unexpectedIOWrite);
    vm.RegisterMMIOReadCallback(unexpectedMMIORead);
    vm.RegisterMMIOWriteCallback(unexpectedMMIOWrite);

    // Run CPU until 8-bit IN
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::PIO:
        printf("Emulation exited due to I/O as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }
    
    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");


    // Setup I/O callbacks
    vm.RegisterIOReadCallback(unexpectedIORead);
    vm.RegisterIOWriteCallback([](void *, uint16_t port, size_t size, uint32_t value) noexcept {
        printf("I/O write callback reached!\n");
        if (port == 0x1001 && size == 1) {
            printf("And we got the right port and size!\n");
            if (value == 0x53) {
                printf("And the right result too!\n");
            }
        }
    });
    vm.RegisterMMIOReadCallback(unexpectedMMIORead);
    vm.RegisterMMIOWriteCallback(unexpectedMMIOWrite);

    // Run CPU until 8-bit OUT
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::PIO:
        printf("Emulation exited due to I/O as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");


    // Setup I/O callbacks
    vm.RegisterIOReadCallback([](void *, uint16_t port, size_t size) noexcept -> uint32_t {
        printf("I/O read callback reached!\n");
        if (port == 0x1002 && size == 2) {
            printf("And we got the right port and size!\n");
            return 0xfade;
        }
        return 0;
    });
    vm.RegisterIOWriteCallback(unexpectedIOWrite);
    vm.RegisterMMIOReadCallback(unexpectedMMIORead);
    vm.RegisterMMIOWriteCallback(unexpectedMMIOWrite);

    // Run CPU until 16-bit IN
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::PIO:
        printf("Emulation exited due to I/O as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");


    // Setup I/O callbacks
    vm.RegisterIOReadCallback(unexpectedIORead);
    vm.RegisterIOWriteCallback([](void *, uint16_t port, size_t size, uint32_t value) noexcept {
        printf("I/O write callback reached!\n");
        if (port == 0x1003 && size == 2) {
            printf("And we got the right port and size!\n");
            if (value == 0x0521) {
                printf("And the right result too!\n");
            }
        }
    });
    vm.RegisterMMIOReadCallback(unexpectedMMIORead);
    vm.RegisterMMIOWriteCallback(unexpectedMMIOWrite);

    // Run CPU until 16-bit OUT
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::PIO:
        printf("Emulation exited due to I/O as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");


    // Setup I/O callbacks
    vm.RegisterIOReadCallback([](void *, uint16_t port, size_t size) noexcept -> uint32_t {
        printf("I/O read callback reached!\n");
        if (port == 0x1004 && size == 4) {
            printf("And we got the right port and size!\n");
            return 0xfeedbabe;
        }
        return 0;
    });
    vm.RegisterIOWriteCallback(unexpectedIOWrite);
    vm.RegisterMMIOReadCallback(unexpectedMMIORead);
    vm.RegisterMMIOWriteCallback(unexpectedMMIOWrite);
    
    // Run CPU until 32-bit IN
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::PIO:
        printf("Emulation exited due to I/O as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");


    // Setup I/O callbacks
    vm.RegisterIOReadCallback(unexpectedIORead);
    vm.RegisterIOWriteCallback([](void *, uint16_t port, size_t size, uint32_t value) noexcept {
        printf("I/O write callback reached!\n");
        if (port == 0x1005 && size == 4) {
            printf("And we got the right port and size!\n");
            if (value == 0x01124541) {
                printf("And the right result too!\n");
            }
        }
    });
    vm.RegisterMMIOReadCallback(unexpectedMMIORead);
    vm.RegisterMMIOWriteCallback(unexpectedMMIOWrite);

    // Run CPU until 32-bit OUT
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::PIO:
        printf("Emulation exited due to I/O as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");

    // ----- MMIO -------------------------------------------------------------------------------------------------------------

    printf("Testing MMIO\n\n");

    // Setup I/O callbacks
    vm.RegisterIOReadCallback(unexpectedIORead);
    vm.RegisterIOWriteCallback(unexpectedIOWrite);
    vm.RegisterMMIOReadCallback([](void *, uint64_t address, size_t size) noexcept -> uint64_t {
        printf("MMIO read callback reached!\n");
        if (address == 0xe0000000 && size == 4) {
            printf("And we got the right address and size!\n");
            return 0xbaadc0de;
        }
        return 0;
    });
    vm.RegisterMMIOWriteCallback(unexpectedMMIOWrite);

    // Run CPU until the first MMIO
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::MMIO:
        printf("Emulation exited due to MMIO as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }
    
    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");


    // Setup I/O callbacks
    vm.RegisterIOReadCallback(unexpectedIORead);
    vm.RegisterIOWriteCallback(unexpectedIOWrite);
    vm.RegisterMMIOReadCallback(unexpectedMMIORead);
    vm.RegisterMMIOWriteCallback([](void *, uint64_t address, size_t size, uint64_t value) noexcept {
        printf("MMIO write callback reached!\n");
        if (address == 0xe0000004 && size == 4) {
            printf("And we got the right address and size!\n");
            if (value == 0xbaadc0de) {
                printf("And the right value too!\n");
            }
        }
    });

    // Will now hit the MMIO read
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::MMIO:
        printf("Emulation exited due to MMIO as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");
    

    // Setup I/O callbacks
    vm.RegisterIOReadCallback(unexpectedIORead);
    vm.RegisterIOWriteCallback(unexpectedIOWrite);
    vm.RegisterMMIOReadCallback([](void *, uint64_t address, size_t size) noexcept -> uint64_t {
        printf("MMIO read callback reached!\n");
        if (address == 0xe0000004 && size == 4) {
            printf("And we got the right address and size!\n");
            return 0xdeadc0de;
        }
        return 0;
    });
    vm.RegisterMMIOWriteCallback([](void *, uint64_t address, size_t size, uint64_t value) noexcept {
        printf("MMIO write callback reached!\n");
        if (address == 0xe0000004 && size == 4) {
            printf("And we got the right address and size!\n");
            if (value == 0xdeadc0de) {
                printf("And the right value too!\n");
            }
        }
    });

    // Will now hit the first part of TEST instruction with MMIO address
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    switch (exitInfo.reason) {
    case VMExitReason::MMIO:
        printf("Emulation exited due to MMIO as expected!\n");
        break;
    default:
        printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
        break;
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");

    // Some platforms require multiple executions to complete an emulated MMIO instruction
    if (features.partialMMIOInstructions) {
        printf("Hypervisor instruction emulator executes MMIO instructions partially, continuing execution...\n\n");

        execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to run\n");
            return -1;
        }

        switch (exitInfo.reason) {
        case VMExitReason::MMIO:
            printf("Emulation exited due to MMIO as expected!\n");
            break;
        default:
            printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
            break;
        }

        printf("\nCPU register state:\n");
        printRegs(vp);
        printf("\n");
    }
    
    // ----- Guest debugging --------------------------------------------------------------------------------------------------

    if (!features.guestDebugging) {
        printf("Guest debugging not supported by the platform, skipping tests\n");
        vp.RegWrite(Reg::EIP, 0x10000085);
    }
    else {
        // ----- Single stepping ----------------------------------------------------------------------------------------------

        printf("Testing single stepping\n\n");

        // Step CPU
        execStatus = vp.Step();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to step\n");
            return -1;
        }

        // Some hypervisors may not step forward after completing the complex
        // MMIO instruction from the previous test. Check if that's the case by
        // looking at EIP
        {
            RegValue eip;
            vp.RegRead(Reg::EIP, eip);
            if (eip.u32 == 0x10000058) {
                printf("Hypervisor does not complete complex MMIO instruction on execution, stepping again\n");
                execStatus = vp.Step();
                if (execStatus != VPExecutionStatus::OK) {
                    printf("VCPU failed to step\n");
                    return -1;
                }
            }
        }

        switch (exitInfo.reason) {
        case VMExitReason::Step:
        {
            printf("Emulation exited due to single stepping as expected!\n");

            RegValue eip, ecx;
            RegValue dr6, dr7;

            vp.RegRead(Reg::EIP, eip);
            vp.RegRead(Reg::ECX, ecx);
            vp.RegRead(Reg::DR6, dr6);
            vp.RegRead(Reg::DR7, dr7);

            if (eip.u32 == 0x1000005d) {
                printf("And stopped at the right place!\n");
            }
            if (ecx.u32 == 0x11) {
                printf("And got the right result!\n");
            }
            break;
        }
        default:
            printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
            break;
        }

        printf("\nCPU register state:\n");
        printRegs(vp);
        printf("\n");


        // Step CPU
        execStatus = vp.Step();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to step\n");
            return -1;
        }

        switch (exitInfo.reason) {
        case VMExitReason::Step:
        {
            printf("Emulation exited due to single stepping as expected!\n");

            RegValue eip, ecx;
            RegValue dr6, dr7;

            vp.RegRead(Reg::EIP, eip);
            vp.RegRead(Reg::ECX, ecx);
            vp.RegRead(Reg::DR6, dr6);
            vp.RegRead(Reg::DR7, dr7);

            if (eip.u32 == 0x10000062) {
                printf("And stopped at the right place!\n");
            }
            if (ecx.u32 == 0x2200) {
                printf("And got the right result!\n");
            }
            break;
        }
        default:
            printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
            break;
        }

        printf("\nCPU register state:\n");
        printRegs(vp);
        printf("\n");


        // Step CPU
        execStatus = vp.Step();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to step\n");
            return -1;
        }

        switch (exitInfo.reason) {
        case VMExitReason::Step:
        {
            printf("Emulation exited due to single stepping as expected!\n");

            RegValue eip, ecx;
            RegValue dr6, dr7;

            vp.RegRead(Reg::EIP, eip);
            vp.RegRead(Reg::ECX, ecx);
            vp.RegRead(Reg::DR6, dr6);
            vp.RegRead(Reg::DR7, dr7);

            if (eip.u32 == 0x10000067) {
                printf("And stopped at the right place!\n");
            }
            if (ecx.u32 == 0x330000) {
                printf("And got the right result!\n");
            }
            break;
        }
        default:
            printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
            break;
        }

        printf("\nCPU register state:\n");
        printRegs(vp);
        printf("\n");


        // Step CPU
        execStatus = vp.Step();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to step\n");
            return -1;
        }

        switch (exitInfo.reason) {
        case VMExitReason::Step:
        {
            printf("Emulation exited due to single stepping as expected!\n");

            RegValue eip, ecx;
            RegValue dr6, dr7;

            vp.RegRead(Reg::EIP, eip);
            vp.RegRead(Reg::ECX, ecx);
            vp.RegRead(Reg::DR6, dr6);
            vp.RegRead(Reg::DR7, dr7);

            if (eip.u32 == 0x1000006c) {
                printf("And stopped at the right place!\n");
            }
            if (ecx.u32 == 0x44000000) {
                printf("And got the right result!\n");
            }
            break;
        }
        default:
            printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
            break;
        }

        printf("\nCPU register state:\n");
        printRegs(vp);
        printf("\n");

        // ----- Software breakpoints -----------------------------------------------------------------------------------------

        // Enable software breakpoints and place a breakpoint
        opStatus = vp.EnableSoftwareBreakpoints(true);
        if (opStatus != VPOperationStatus::OK) {
            printf("Failed to enable software breakpoints\n");
            return -1;
        }
        const uint8_t swBpBackup = ram[0x5071];
        ram[0x5071] = 0xCC;

        // Run CPU. Should hit the breakpoint
        execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to run\n");
            return -1;
        }

        switch (exitInfo.reason) {
        case VMExitReason::SoftwareBreakpoint:
        {
            RegValue eip, ecx;
            RegValue dr6, dr7;
            uint64_t bpAddr;

            vp.RegRead(Reg::EIP, eip);
            vp.RegRead(Reg::ECX, ecx);
            vp.RegRead(Reg::DR6, dr6);
            vp.RegRead(Reg::DR7, dr7);

            vp.GetBreakpointAddress(&bpAddr);

            printf("Emulation exited due to software breakpoint as expected!\n");
            if (bpAddr == 0x10000071) {
                printf("And triggered the correct breakpoint!\n");
            }
            if (eip.u32 == 0x10000071) {
                printf("And stopped at the right place!\n");
            }
            if (ecx.u32 == 0x000000ff) {
                printf("And got the right result!\n");
            }
            break;
        }
        default:
            printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
            break;
        }

        printf("\nCPU register state:\n");
        printRegs(vp);
        printf("\n");


        // Disable software breakpoints and revert instruction
        opStatus = vp.EnableSoftwareBreakpoints(false);
        if (opStatus != VPOperationStatus::OK) {
            printf("Failed to disable software breakpoints\n");
            return -1;
        }
        ram[0x5071] = swBpBackup;

        // ----- Hardware breakpoints -----------------------------------------------------------------------------------------

        // Place hardware breakpoint
        HardwareBreakpoints bps = { 0 };
        bps.bp[0].address = 0x1000007b;
        bps.bp[0].localEnable = true;
        bps.bp[0].globalEnable = false;
        bps.bp[0].trigger = HardwareBreakpointTrigger::Execution;
        bps.bp[0].length = HardwareBreakpointLength::Byte;
        opStatus = vp.SetHardwareBreakpoints(bps);
        if (opStatus != VPOperationStatus::OK) {
            printf("Failed to set hardware breakpoint\n");
            return -1;
        }

        // Run CPU. Should hit the breakpoint
        execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to run\n");
            return -1;
        }

        switch (exitInfo.reason) {
        case VMExitReason::HardwareBreakpoint:
        {
            printf("Emulation exited due to hardware breakpoint as expected!\n");
            RegValue eip, ecx, dr6, dr7;

            vp.RegRead(Reg::EIP, eip);
            vp.RegRead(Reg::ECX, ecx);
            vp.RegRead(Reg::DR6, dr6);
            vp.RegRead(Reg::DR7, dr7);

            if (dr6.u32 == 1) {
                printf("And triggered the correct breakpoint!\n");
            }
            if (eip.u32 == 0x1000007b) {
                printf("And stopped at the right place!\n");
            }
            if (ecx.u32 == 0x00dd0000) {
                printf("And got the right result!\n");
            }
            break;
        }
        default:
            printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
            break;
        }

        // Clear hardware breakpoints
        opStatus = vp.ClearHardwareBreakpoints();
        if (opStatus != VPOperationStatus::OK) {
            printf("Could not clear hardware breakpoints\n");
        }
        printf("\nHardware breakpoints cleared\n");
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");

    // ----- Extended VM exit: CPUID ------------------------------------------------------------------------------------------

    if (extVMExits.NoneOf(ExtendedVMExit::CPUID)) {
        printf("Extended VM exit on CPUID instruction not supported by the platform, skipping test\n");
        vp.RegWrite(Reg::EIP, 0x10000091);
    }
    else {
        printf("Testing extended VM exit: CPUID instruction\n\n");

        // Run CPU. Should hit the CPUID and exit with the correct result
        execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to run\n");
            return -1;
        }

        switch (exitInfo.reason) {
        case VMExitReason::CPUID:
        {
            printf("Emulation exited due to CPUID instruction as expected!\n");
            
            RegValue eax;
            vp.RegRead(Reg::EAX, eax);
            if (eax.u32 == 0) {
                printf("And we got the correct function!\n");

                vp.RegWrite(Reg::EAX, 0x80000008);
                vp.RegWrite(Reg::EBX, 'vuoc');
                vp.RegWrite(Reg::ECX, 'Rtri');
                vp.RegWrite(Reg::EDX, 'SKCO');
            }
            break;
        }
        default:
            printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
            break;
        }

        printf("\nCPU register state:\n");
        printRegs(vp);
        printf("\n");


        // Should hit the next CPUID with function 0x800000002 then stop at the
        // following HLT
        execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("VCPU failed to run\n");
            return -1;
        }

        switch (exitInfo.reason) {
        case VMExitReason::HLT:
        {
            printf("Emulation exited due to HLT instruction as expected!\n");

            RegValue eax, ebx, ecx, edx;
            vp.RegRead(Reg::EAX, eax);
            vp.RegRead(Reg::EBX, ebx);
            vp.RegRead(Reg::ECX, ecx);
            vp.RegRead(Reg::EDX, edx);
            if (eax.u32 == 'vupc' && ebx.u32 == ' tri' && ecx.u32 == 'UPCV' && edx.u32 == '    ') {
                printf("And we got the correct results!\n");
            }
            else if (features.customCPUIDs) {
                printf("Custom CPUID results unsupported by the hypervisor\n");
            }
            break;
        }
        default:
            printf("Emulation exited for another reason: %s\n", reason_str(exitInfo.reason));
            break;
        }
    }

    printf("\nCPU register state:\n");
    printRegs(vp);
    printf("\n");
    
    // ----- End of the program -----------------------------------------------------------------------------------------------

    // Run CPU. Will stop at the last HLT instruction
    execStatus = vp.Run();
    if (execStatus != VPExecutionStatus::OK) {
        printf("VCPU failed to run\n");
        return -1;
    }

    // Validate
    {
        RegValue eip;
        vp.RegRead(Reg::EIP, eip);
        if (eip.u32 == 0x10000092) {
            printf("Emulation stopped at the right place!\n");
        }
    }

    printf("\nFinal CPU register state:\n");
    printRegs(vp);
    printFPRegs(vp);
    printSSERegs(vp);
    printf("\n");

    // ----- Linear memory address translation --------------------------------------------------------------------------------

    printf("Linear memory address translations:\n");
    printAddressTranslation(vp, 0x00000000);
    printAddressTranslation(vp, 0x00001000);
    printAddressTranslation(vp, 0x00010000);
    printAddressTranslation(vp, 0x10000000);
    printAddressTranslation(vp, 0x10001000);
    printAddressTranslation(vp, 0xe0000000);
    printAddressTranslation(vp, 0xffffe000);
    printAddressTranslation(vp, 0xfffff000);

    // ----- Cleanup ----------------------------------------------------------------------------------------------------------

    printf("\n");
   
    // Free VM
    printf("Releasing VM... ");
    if (platform.FreeVM(vm)) {
        printf("succeeded\n");
    }
    else {
        printf("failed\n");
    }

    // Free RAM
    if (alignedFree(ram)) {
        printf("RAM freed\n");
    }
    else {
        printf("Failed to free RAM\n");
    }

    // Free ROM
    if (alignedFree(rom)) {
        printf("ROM freed\n");
    }
    else {
        printf("Failed to free ROM\n");
    }

    return 0;
}

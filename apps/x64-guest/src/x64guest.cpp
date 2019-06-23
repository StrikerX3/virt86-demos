/*
Entry point of the 64-bit guest demo.

Creates a virtual machine that switches to 64-bit mode from real mode.
Based on instruction from https://wiki.osdev.org/Entering_Long_Mode_Directly
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

int main(int argc, char* argv[]) {
    // Require two arguments: the ROM code and the RAM code
    if (argc < 3) {
        printf("fatal: no input files specified\n");
        printf("usage: %s <rom> <ram>\n", argv[0]);
        return -1;
    }

    // ROM and RAM sizes
    const uint32_t romSize = PAGE_SIZE * 16;  // 64 KiB
    const uint32_t ramSize = PAGE_SIZE * 512; // 2 MiB
    const uint64_t romBase = 0xFFFF0000;
    const uint64_t ramBase = 0x0;
    const uint64_t ramProgramBase = 0x10000;

    // --- ROM ------------------

    // Initialize ROM
    uint8_t *rom = alignedAlloc(romSize);
    if (rom == NULL) {
        printf("fatal: failed to allocate memory for ROM\n");
        return -1;
    }
    printf("ROM allocated: %u bytes\n", romSize);

    // Open ROM file specified in the command line
    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("fatal: could not open ROM file: %s\n", argv[1]);
        return -1;
    }

    // Check that the ROM file is exactly 64 KiB
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (len != romSize) {
        printf("fatal: ROM file size must be %ud bytes\n", romSize);
        return -1;
    }
    
    // Load ROM from the file
    size_t readLen = fread(rom, 1, len, fp);
    if (readLen != len) {
        printf("fatal: could not fully read ROM file\n");
        return -1;
    }
    fclose(fp);
    printf("ROM loaded from %s\n", argv[1]);

    // --- RAM ------------------

    // Initialize and clear RAM
    uint8_t *ram = alignedAlloc(ramSize);
    if (ram == NULL) {
        printf("fatal: failed to allocate memory for RAM\n");
        return -1;
    }
    memset(ram, 0, ramSize);
    printf("RAM allocated: %u bytes\n", ramSize);
    
    // Open RAM file specified in the command line
    fp = fopen(argv[2], "rb");
    if (fp == NULL) {
        printf("fatal: could not open RAM file: %s\n", argv[2]);
        return -1;
    }

    // Check that the RAM file fits in our RAM
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (len > ramSize - ramProgramBase) {
        printf("fatal: RAM file size must be no larger than %llu bytes\n", ramSize - ramProgramBase);
        return -1;
    }

    // Load RAM from the file
    readLen = fread(&ram[0x10000], 1, len, fp);
    if (readLen != len) {
        printf("fatal: could not fully read RAM file\n");
        return -1;
    }
    fclose(fp);
    printf("RAM loaded from %s\n", argv[2]);
    
    printf("\n");

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
    auto& features = platform.GetFeatures();
    
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
    default: printf("failed: unhandled reason (%d)\n", static_cast<int>(memMapStatus)); return -1;
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

    // The ROM code expects the following:
    //   es:edi    Should point to a valid page-aligned 16KiB buffer, for the PML4, PDPT, PD and a PT.
    //   ss:esp    Should point to memory that can be used as a small (1 uint32_t) stack
    // We'll set up our page table at 0x0 and use 0x10000 as the base of our stack, just below the user program.
    {
        RegValue edi, esp;
        edi.u32 = 0x0;
        esp.u32 = 0x10000;
        vp.RegWrite(Reg::EDI, edi);
        vp.RegWrite(Reg::ESP, esp);
    }

    // Run until HLT is reached
    bool running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printf("\n");

        auto& exitInfo = vp.GetVMExitInfo();
        switch (exitInfo.reason) {
        case VMExitReason::HLT:
            printf("HLT reached\n");
            running = false;
            break;
        case VMExitReason::Shutdown:
            printf("VCPU shutting down\n");
            running = false;
            break;
        case VMExitReason::Error:
            printf("VCPU execution failed\n");
            running = false;
            break;
        }
    }

    printf("\n");

    // Map a page of memory to the guest and write some data to be read by the guest in order to check if the mapping worked

    // Values written to the newly allocated pages
    const static uint64_t checkValue1 = 0xfedcba9876543210;
    const static uint64_t checkValue2 = 0x0123456789abcdef;

    // Allocate host memory for the new page and write the check value to its base address
    const uint64_t moreRamBase = 0x11800024000;  // This can be any physical address that's not already occupied by RAM or ROM; must be page-aligned
    const size_t moreRamSize = PAGE_SIZE * 2;
    uint8_t* moreRam = alignedAlloc(moreRamSize);
    if (moreRam == NULL) {
        printf("fatal: failed to allocate memory for additional RAM\n");
        return -1;
    }
    memset(moreRam, 0, moreRamSize);
    printf("Additional RAM allocated: %u bytes\n", moreRamSize);
    memcpy(moreRam, &checkValue1, sizeof(checkValue1));
    memcpy(moreRam + PAGE_SIZE, &checkValue2, sizeof(checkValue2));

    // Map the memory to the guest at the desired base address
    printf("Mapping additional RAM... ");
    memMapStatus = vm.MapGuestMemory(moreRamBase, moreRamSize, MemoryFlags::Read | MemoryFlags::Write | MemoryFlags::Execute, moreRam);
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
    default: printf("failed: unhandled reason (%d)\n", static_cast<int>(memMapStatus)); return -1;
    }

    // Map the newly added physical page to linear address 0x100000000
    // PML4E for that virtual address already exists
    *(uint64_t*)&ram[0x1020] = 0x5023;  // PDPTE -> PDE at 0x5000
    *(uint64_t*)&ram[0x5000] = 0x6023;  // PDE -> PTE at 0x6000
    *(uint64_t*)&ram[0x6000] = (moreRamBase & ~0xFFF) | 0x23;   // PTE -> physical address

    // Display linear-to-physical address translation of the new page
    printAddressTranslation(vp, 0x100000000);
    printf("\n");

    // Run until HLT is reached
    running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printf("\n");

        auto& exitInfo = vp.GetVMExitInfo();
        switch (exitInfo.reason) {
        case VMExitReason::HLT:
            printf("HLT reached\n");
            {
                RegValue rax;
                vp.RegRead(Reg::RAX, rax);
                if (rax.u64 == checkValue1) {
                    printf("Got the right value\n");
                }
            }
            running = false;
            break;
        case VMExitReason::Shutdown:
            printf("VCPU shutting down\n");
            running = false;
            break;
        case VMExitReason::Error:
            printf("VCPU execution failed\n");
            running = false;
            break;
        }
    }

    printf("\n");

    // Update page mapping to point to the second page of the newly allocated RAM
    *(uint64_t*)&ram[0x6000] = ((moreRamBase & ~0xFFF) + 0x1000) | 0x23;   // PTE -> physical address

    // Display new address translation
    printf("Page mapping updated:\n");
    printAddressTranslation(vp, 0x100000000);
    printf("\n");

    // Run until HLT is reached
    running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printf("\n");

        auto& exitInfo = vp.GetVMExitInfo();
        switch (exitInfo.reason) {
        case VMExitReason::HLT:
            printf("HLT reached\n");
            {
                RegValue rax;
                vp.RegRead(Reg::RAX, rax);
                if (rax.u64 == checkValue2) {
                    printf("Got the right value\n");
                }
            }
            running = false;
            break;
        case VMExitReason::Shutdown:
            printf("VCPU shutting down\n");
            running = false;
            break;
        case VMExitReason::Error:
            printf("VCPU execution failed\n");
            running = false;
            break;
        }
    }

    printf("\n");

    // Debugging
    {
		printf("Final VCPU state:\n");
		printRegs(vp);
		printf("\n");

		printf("Linear memory address translations:\n");
        printAddressTranslation(vp, 0x00000000);
        printAddressTranslation(vp, 0x00010000);
        printAddressTranslation(vp, 0xffff0000);
        printAddressTranslation(vp, 0xffff00e8);
        printAddressTranslation(vp, 0x100000000);
        printf("\n");

		uint64_t stackVal;
		if (vp.LMemRead(0x200000 - 8, sizeof(uint64_t), &stackVal)) {
			printf("Value written to stack: 0x%016" PRIx64 "\n", stackVal);
		}
        
        RegValue gdtr, idtr;
        vp.RegRead(Reg::GDTR, gdtr);
        vp.RegRead(Reg::IDTR, idtr);

        GDTEntry gdtCode, gdtData;
		vp.GetGDTEntry(0x0008, gdtCode);
		vp.GetGDTEntry(0x0010, gdtData);
		printf("Code GDT: base=0x%08x, limit=0x%08x, access=0x%02x, flags=0x%x\n", gdtCode.gdt.GetBase(), gdtCode.gdt.GetLimit(), gdtCode.gdt.data.access.u8, gdtCode.gdt.data.flags);
		printf("Data GDT: base=0x%08x, limit=0x%08x, access=0x%02x, flags=0x%x\n", gdtData.gdt.GetBase(), gdtData.gdt.GetLimit(), gdtData.gdt.data.access.u8, gdtData.gdt.data.flags);

        printf("\n");
    }

    // ----- Cleanup ----------------------------------------------------------------------------------------------------------
   
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

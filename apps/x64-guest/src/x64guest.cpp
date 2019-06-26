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

#include <cmath>

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
    {
        auto memMapStatus = vm.MapGuestMemory(romBase, romSize, MemoryFlags::Read | MemoryFlags::Execute, rom);
        printMemoryMappingStatus(memMapStatus);
        if (memMapStatus != MemoryMappingStatus::OK) return -1;
    }

    // Map RAM to the bottom of the 32-bit address range
    printf("Mapping RAM... ");
    {
        auto memMapStatus = vm.MapGuestMemory(ramBase, ramSize, MemoryFlags::Read | MemoryFlags::Write | MemoryFlags::Execute | MemoryFlags::DirtyPageTracking, ram);
        printMemoryMappingStatus(memMapStatus);
        if (memMapStatus != MemoryMappingStatus::OK) return -1;
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

    // ----- Start ----------------------------------------------------------------------------------------------------

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

    // ----- Page table manipulation ----------------------------------------------------------------------------------
    
    // Map a page of memory to the guest and write some data to be read by the guest in order to check if the mapping worked

    // Values written to the newly allocated pages
    const static uint64_t checkValue1 = 0xfedcba9876543210;
    const static uint64_t checkValue2 = 0x0123456789abcdef;

    // Allocate host memory for the new page and write the check value to its base address
    // We allocate near the top of the maximum supported GPA, with one page of breathing room because HAXM doesn't let us use the last page
    const size_t moreRamSize = PAGE_SIZE * 2;
    const uint64_t moreRamBase = features.guestPhysicalAddress.maxAddress - moreRamSize - 0x1000;
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
    printf("Mapping additional RAM to 0x%llx... ", moreRamBase);
    {
        auto memMapStatus = vm.MapGuestMemory(moreRamBase, moreRamSize, MemoryFlags::Read | MemoryFlags::Write | MemoryFlags::Execute, moreRam);
        printMemoryMappingStatus(memMapStatus);
        if (memMapStatus != MemoryMappingStatus::OK) return -1;
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

    // Define some helper functions for tests
    static constexpr float float_epsilon = 1e-5f;
    auto feq = [](float x, float y) -> bool { return fabs(x - y) <= float_epsilon; };

    static constexpr double double_epsilon = 1e-9f;
    auto deq = [](double x, double y) -> bool { return fabs(x - y) <= double_epsilon; };

    // ----- MMX ------------------------------------------------------------------------------------------------------

    // Run until HLT is reached
    running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printMMRegs(vp, MMFormat::I16);
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
   
    // Check result
    {
        RegValue rax, rsi, mm0;
        vp.RegRead(Reg::RAX, rax);
        vp.RegRead(Reg::RSI, rsi);   // contains address of result in memory
        vp.RegRead(Reg::MM0, mm0);

        uint64_t memValue;
        vp.LMemRead(rsi.u64, sizeof(memValue), &memValue);
        
        if (rax.u64 == 0x002c00210016000b) printf("RAX contains the correct result\n");
        if (memValue == 0x002c00210016000b) printf("Memory contains the correct result\n");
        if (mm0.mm.i64[0] == 0x002c00210016000b) printf("MM0 contains the correct result\n");
        printf("MMX test complete\n");
    }

    printf("\n");

    // ----- SSE ------------------------------------------------------------------------------------------------------

    // Run until HLT is reached
    running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printXMMRegs(vp, XMMFormat::F32);
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

    // Check result
    {
        RegValue rax, rsi, xmm0;
        vp.RegRead(Reg::RAX, rax);
        vp.RegRead(Reg::RSI, rsi);   // contains address of result in memory
        vp.RegRead(Reg::XMM0, xmm0);

        float memValue[4];
        vp.LMemRead(rsi.u64, sizeof(memValue), &memValue);

        // Reinterpret RAX as if it were the lowest 64 bits of XMM0
        if (feq(rax.xmm.f32[0], 30.8) && feq(rax.xmm.f32[1], 51.48)) printf("RAX contains the correct result\n");
        if (feq(memValue[0], 30.8) && feq(memValue[1], 51.48) && feq(memValue[2], 77.0) && feq(memValue[3], 107.36)) printf("Memory contains the correct result\n");
        if (feq(xmm0.xmm.f32[0], 30.8) && feq(xmm0.xmm.f32[1], 51.48) && feq(xmm0.xmm.f32[2], 77.0) && feq(xmm0.xmm.f32[3], 107.36)) printf("XMM0 contains the correct result\n");
        printf("SSE test complete\n");
    }

    printf("\n");

    // ----- SSE2 -----------------------------------------------------------------------------------------------------

    // Run until HLT is reached
    running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printXMMRegs(vp, XMMFormat::F64);
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

    // Check result
    {
        RegValue rax, rsi, xmm0;
        vp.RegRead(Reg::RAX, rax);
        vp.RegRead(Reg::RSI, rsi);   // contains address of result in memory
        vp.RegRead(Reg::XMM0, xmm0);

        double memValue[4];
        vp.LMemRead(rsi.u64, sizeof(memValue), &memValue);

        // Reinterpret RAX as if it were the lowest 64 bits of XMM0
        if (deq(rax.xmm.f64[0], 11.22)) printf("RAX contains the correct result\n");
        if (deq(memValue[0], 11.22) && deq(memValue[1], 24.64)) printf("Memory contains the correct result\n");
        if (deq(xmm0.xmm.f64[0], 11.22) && deq(xmm0.xmm.f64[1], 24.64)) printf("XMM0 contains the correct result\n");
        printf("SSE2 test complete\n");
    }

    printf("\n");

    // ----- SSE3 -----------------------------------------------------------------------------------------------------

    // Run until HLT is reached
    running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printXMMRegs(vp, XMMFormat::F64);
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

    // Check result
    {
        RegValue rax, rsi, xmm0;
        vp.RegRead(Reg::RAX, rax);
        vp.RegRead(Reg::RSI, rsi);   // contains address of result in memory
        vp.RegRead(Reg::XMM0, xmm0);

        double memValue[4];
        vp.LMemRead(rsi.u64, sizeof(memValue), &memValue);

        // Reinterpret RAX as if it were the lowest 64 bits of XMM0
        if (deq(rax.xmm.f64[0], 4.0)) printf("RAX contains the correct result\n");
        if (deq(memValue[0], 4.0) && deq(memValue[1], 2.0)) printf("Memory contains the correct result\n");
        if (deq(xmm0.xmm.f64[0], 4.0) && deq(xmm0.xmm.f64[1], 2.0)) printf("XMM0 contains the correct result\n");
        printf("SSE3 test complete\n");
    }

    printf("\n");

    // ----- SSSE3 ----------------------------------------------------------------------------------------------------

    // Run until HLT is reached
    running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printXMMRegs(vp, XMMFormat::I32);
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

    // Check result
    {
        RegValue rax, rsi, xmm1;
        vp.RegRead(Reg::RAX, rax);
        vp.RegRead(Reg::RSI, rsi);   // contains address of result in memory
        vp.RegRead(Reg::XMM1, xmm1);

        int32_t memValue[4];
        vp.LMemRead(rsi.u64, sizeof(memValue), &memValue);

        // Reinterpret RAX as if it were the lowest 64 bits of XMM1
        if (rax.xmm.i32[0] == -3087 && rax.xmm.i32[1] == 3087) printf("RAX contains the correct result\n");
        if (memValue[0] == -3087 && memValue[1] == 3087 && memValue[2] == 5555 && memValue[3] == 5555) printf("Memory contains the correct result\n");
        if (xmm1.xmm.i32[0] == -3087 && xmm1.xmm.i32[1] == 3087 && xmm1.xmm.i32[2] == 5555 && xmm1.xmm.i32[3] == 5555) printf("XMM1 contains the correct result\n");
        printf("SSSE3 test complete\n");
    }

    printf("\n");

    // ----- SSE4 -----------------------------------------------------------------------------------------------------

    // Run until HLT is reached
    running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printXMMRegs(vp, XMMFormat::I64);
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

    // Check result
    {
        RegValue rax, rsi, xmm2;
        vp.RegRead(Reg::RAX, rax);
        vp.RegRead(Reg::RSI, rsi);   // contains address of result in memory
        vp.RegRead(Reg::XMM2, xmm2);

        int64_t memValue[2];
        vp.LMemRead(rsi.u64, sizeof(memValue), &memValue);

        if (rax.u64 == 0) printf("RAX contains the correct result\n");
        if (memValue[0] == 0 && memValue[1] == -1) printf("Memory contains the correct result\n");
        if (xmm2.xmm.i64[0] == 0 && xmm2.xmm.i64[1] == -1) printf("XMM2 contains the correct result\n");
        printf("SSE4 test complete\n");
    }

    printf("\n");

    // ----- AVX ------------------------------------------------------------------------------------------------------

    // Run until HLT is reached
    running = true;
    while (running) {
        auto execStatus = vp.Run();
        if (execStatus != VPExecutionStatus::OK) {
            printf("Virtual CPU execution failed\n");
            break;
        }

        printRegs(vp);
        printXMMRegs(vp, XMMFormat::F32);
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

    // Check result
    {
        RegValue rax, rsi, xmm3;
        vp.RegRead(Reg::RAX, rax);
        vp.RegRead(Reg::RSI, rsi);   // contains address of result in memory
        vp.RegRead(Reg::XMM3, xmm3);  // TODO: should read YMM3, but no hypervisors support that so far

        float memValue[8];
        vp.LMemRead(rsi.u64, sizeof(memValue), &memValue);

        // Reinterpret RAX as if it were the lowest 64 bits of XMM3
        if (feq(rax.xmm.f32[0], 10.0) && feq(rax.xmm.f32[1], 15.0)) printf("RAX contains the correct result\n");
        if (feq(memValue[0], 10.0) && feq(memValue[1], 15.0) && feq(memValue[2], 20.0) && feq(memValue[3], 25.0)
            && feq(memValue[4], 30.0) && feq(memValue[5], 35.0) && feq(memValue[6], 40.0) && feq(memValue[7], 45.0)) printf("Memory contains the correct result\n");
        if (feq(xmm3.xmm.f32[0], 10.0) && feq(xmm3.xmm.f32[1], 15.0) && feq(xmm3.xmm.f32[2], 20.0) && feq(xmm3.xmm.f32[3], 25.0)) printf("XMM3 contains the correct result\n");
        printf("AVX test complete\n");
    }

    printf("\n");
    
    // TODO: continue implementing test verifications

    // ----- End ------------------------------------------------------------------------------------------------------

    printf("Final VCPU state:\n");
    printRegs(vp);
    printSTRegs(vp);
    printMMRegs(vp, MMFormat::I16);
    printMXCSRRegs(vp);
    printXMMRegs(vp, XMMFormat::IF32);
    printYMMRegs(vp, XMMFormat::IF64);
    printZMMRegs(vp, XMMFormat::IF64);
    printf("\n");

    printf("Linear memory address translations:\n");
    printAddressTranslation(vp, 0x00000000);
    printAddressTranslation(vp, 0x00010000);
    printAddressTranslation(vp, 0xffff0000);
    printAddressTranslation(vp, 0xffff00e8);
    printAddressTranslation(vp, 0x100000000);
    printf("\n");

    {
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

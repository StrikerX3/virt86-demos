/*
Defines helper functions that print out formatted register values.
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

#include <cstdio>
#include <cinttypes>

using namespace virt86;

#define PRINT_FLAG(flags, prefix, flag) \
do { \
    if (flags & prefix##_##flag) printf(" " #flag); \
} while (0)

void printRFLAGSBits(uint64_t rflags) noexcept {
    PRINT_FLAG(rflags, RFLAGS, CF);
    PRINT_FLAG(rflags, RFLAGS, PF);
    PRINT_FLAG(rflags, RFLAGS, AF);
    PRINT_FLAG(rflags, RFLAGS, ZF);
    PRINT_FLAG(rflags, RFLAGS, SF);
    PRINT_FLAG(rflags, RFLAGS, TF);
    PRINT_FLAG(rflags, RFLAGS, IF);
    PRINT_FLAG(rflags, RFLAGS, DF);
    PRINT_FLAG(rflags, RFLAGS, OF);
    PRINT_FLAG(rflags, RFLAGS, NT);
    PRINT_FLAG(rflags, RFLAGS, RF);
    PRINT_FLAG(rflags, RFLAGS, VM);
    PRINT_FLAG(rflags, RFLAGS, AC);
    PRINT_FLAG(rflags, RFLAGS, VIF);
    PRINT_FLAG(rflags, RFLAGS, VIP);
    PRINT_FLAG(rflags, RFLAGS, ID);
    const uint8_t iopl = (rflags & RFLAGS_IOPL) >> RFLAGS_IOPL_SHIFT;
    printf(" IOPL=%u", iopl);
}

void printEFERBits(uint64_t efer) noexcept {
    PRINT_FLAG(efer, EFER, SCE);
    PRINT_FLAG(efer, EFER, LME);
    PRINT_FLAG(efer, EFER, LMA);
    PRINT_FLAG(efer, EFER, NXE);
    PRINT_FLAG(efer, EFER, SVME);
    PRINT_FLAG(efer, EFER, LMSLE);
    PRINT_FLAG(efer, EFER, FFXSR);
    PRINT_FLAG(efer, EFER, TCE);
}

void printCR0Bits(uint64_t cr0) noexcept {
    PRINT_FLAG(cr0, CR0, PE);
    PRINT_FLAG(cr0, CR0, MP);
    PRINT_FLAG(cr0, CR0, EM);
    PRINT_FLAG(cr0, CR0, TS);
    PRINT_FLAG(cr0, CR0, ET);
    PRINT_FLAG(cr0, CR0, NE);
    PRINT_FLAG(cr0, CR0, WP);
    PRINT_FLAG(cr0, CR0, AM);
    PRINT_FLAG(cr0, CR0, NW);
    PRINT_FLAG(cr0, CR0, CD);
    PRINT_FLAG(cr0, CR0, PG);
}

void printCR4Bits(uint64_t cr4) noexcept {
    PRINT_FLAG(cr4, CR4, VME);
    PRINT_FLAG(cr4, CR4, PVI);
    PRINT_FLAG(cr4, CR4, TSD);
    PRINT_FLAG(cr4, CR4, DE);
    PRINT_FLAG(cr4, CR4, PSE);
    PRINT_FLAG(cr4, CR4, PAE);
    PRINT_FLAG(cr4, CR4, MCE);
    PRINT_FLAG(cr4, CR4, PGE);
    PRINT_FLAG(cr4, CR4, PCE);
    PRINT_FLAG(cr4, CR4, OSFXSR);
    PRINT_FLAG(cr4, CR4, OSXMMEXCPT);
    PRINT_FLAG(cr4, CR4, UMIP);
    PRINT_FLAG(cr4, CR4, VMXE);
    PRINT_FLAG(cr4, CR4, SMXE);
    PRINT_FLAG(cr4, CR4, PCID);
    PRINT_FLAG(cr4, CR4, OSXSAVE);
    PRINT_FLAG(cr4, CR4, SMEP);
    PRINT_FLAG(cr4, CR4, SMAP);
}

void printCR8Bits(uint64_t cr8) noexcept {
    const uint8_t tpr = cr8 & CR8_TPR;
    printf(" TPR=%u", tpr);
}

void printXCR0Bits(uint64_t xcr0) noexcept {
    PRINT_FLAG(xcr0, XCR0, FP);
    PRINT_FLAG(xcr0, XCR0, SSE);
    PRINT_FLAG(xcr0, XCR0, AVX);
    PRINT_FLAG(xcr0, XCR0, BNDREG);
    PRINT_FLAG(xcr0, XCR0, BNDCSR);
    PRINT_FLAG(xcr0, XCR0, opmask);
    PRINT_FLAG(xcr0, XCR0, ZMM_Hi256);
    PRINT_FLAG(xcr0, XCR0, Hi16_ZMM);
    PRINT_FLAG(xcr0, XCR0, PKRU);
}

void printDR6Bits(uint64_t dr6) noexcept {
    PRINT_FLAG(dr6, DR6, BP0);
    PRINT_FLAG(dr6, DR6, BP1);
    PRINT_FLAG(dr6, DR6, BP2);
    PRINT_FLAG(dr6, DR6, BP3);
}

void printDR7Bits(uint64_t dr7) noexcept {
    for (uint8_t i = 0; i < 4; i++) {
        if (dr7 & (DR7_LOCAL(i) | DR7_GLOBAL(i))) {
            printf(" BP%u[", i);

            if (dr7 & DR7_LOCAL(i)) printf("L");
            if (dr7 & DR7_GLOBAL(i)) printf("G");

            const uint8_t size = (dr7 & DR7_SIZE(i)) >> DR7_SIZE_SHIFT(i);
            switch (size) {
            case DR7_SIZE_BYTE: printf(" byte"); break;
            case DR7_SIZE_WORD: printf(" word"); break;
            case DR7_SIZE_QWORD: printf(" qword"); break;
            case DR7_SIZE_DWORD: printf(" dword"); break;
            }

            const uint8_t cond = (dr7 & DR7_COND(i)) >> DR7_COND_SHIFT(i);
            switch (cond) {
            case DR7_COND_EXEC: printf(" exec"); break;
            case DR7_COND_WIDTH8: printf(" width8"); break;
            case DR7_COND_WRITE: printf(" write"); break;
            case DR7_COND_READWRITE: printf(" r/w"); break;
            }

            printf("]");
        }
    }
}
#undef PRINT_FLAG

void printRegs(VirtualProcessor& vp) noexcept {
#define READREG(code, name) RegValue name; vp.RegRead(code, name);
    READREG(Reg::RAX, rax); READREG(Reg::RCX, rcx); READREG(Reg::RDX, rdx); READREG(Reg::RBX, rbx);
    READREG(Reg::RSP, rsp); READREG(Reg::RBP, rbp); READREG(Reg::RSI, rsi); READREG(Reg::RDI, rdi);
    READREG(Reg::R8, r8); READREG(Reg::R9, r9); READREG(Reg::R10, r10); READREG(Reg::R11, r11);
    READREG(Reg::R12, r12); READREG(Reg::R13, r13); READREG(Reg::R14, r14); READREG(Reg::R15, r15);
    READREG(Reg::RIP, rip);
    READREG(Reg::CS, cs); READREG(Reg::SS, ss);
    READREG(Reg::DS, ds); READREG(Reg::ES, es);
    READREG(Reg::FS, fs); READREG(Reg::GS, gs);
    READREG(Reg::LDTR, ldtr); READREG(Reg::TR, tr);
    READREG(Reg::GDTR, gdtr);
    READREG(Reg::IDTR, idtr);
    READREG(Reg::RFLAGS, rflags);
    READREG(Reg::EFER, efer);
    READREG(Reg::CR2, cr2); READREG(Reg::CR0, cr0);
    READREG(Reg::CR3, cr3); READREG(Reg::CR4, cr4);
    READREG(Reg::DR0, dr0); READREG(Reg::CR8, cr8); 
    READREG(Reg::DR1, dr1); READREG(Reg::XCR0, xcr0);
    READREG(Reg::DR2, dr2); READREG(Reg::DR6, dr6);
    READREG(Reg::DR3, dr3); READREG(Reg::DR7, dr7);
#undef READREG
  
    const auto extendedRegs = BitmaskEnum(vp.GetVirtualMachine().GetPlatform().GetFeatures().extendedControlRegisters);

    printf(" RAX = %016" PRIx64 "   RCX = %016" PRIx64 "   RDX = %016" PRIx64 "   RBX = %016" PRIx64 "\n", rax.u64, rcx.u64, rdx.u64, rbx.u64);
    printf(" RSP = %016" PRIx64 "   RBP = %016" PRIx64 "   RSI = %016" PRIx64 "   RDI = %016" PRIx64 "\n", rsp.u64, rbp.u64, rsi.u64, rdi.u64);
    printf("  R8 = %016" PRIx64 "    R9 = %016" PRIx64 "   R10 = %016" PRIx64 "   R11 = %016" PRIx64 "\n",  r8.u64,  r9.u64, r10.u64, r11.u64);
    printf(" R12 = %016" PRIx64 "   R13 = %016" PRIx64 "   R14 = %016" PRIx64 "   R15 = %016" PRIx64 "\n", r12.u64, r13.u64, r14.u64, r15.u64);
    printf(" RIP = %016" PRIx64 "\n", rip.u64);
    printf("  CS = %04x -> %016" PRIx64 ":%08x [%04x]   SS = %04x -> %016" PRIx64 ":%08x [%04x]\n", cs.segment.selector, cs.segment.base, cs.segment.limit, cs.segment.attributes.u16, ss.segment.selector, ss.segment.base, ss.segment.limit, ss.segment.attributes.u16);
    printf("  DS = %04x -> %016" PRIx64 ":%08x [%04x]   ES = %04x -> %016" PRIx64 ":%08x [%04x]\n", ds.segment.selector, ds.segment.base, ds.segment.limit, ds.segment.attributes.u16, es.segment.selector, es.segment.base, es.segment.limit, es.segment.attributes.u16);
    printf("  FS = %04x -> %016" PRIx64 ":%08x [%04x]   GS = %04x -> %016" PRIx64 ":%08x [%04x]\n", fs.segment.selector, fs.segment.base, fs.segment.limit, fs.segment.attributes.u16, gs.segment.selector, gs.segment.base, gs.segment.limit, gs.segment.attributes.u16);
    printf("LDTR = %04x -> %016" PRIx64 ":%08x [%04x]   TR = %04x -> %016" PRIx64 ":%08x [%04x]\n", ldtr.segment.selector, ldtr.segment.base, ldtr.segment.limit, ldtr.segment.attributes.u16, tr.segment.selector, tr.segment.base, tr.segment.limit, tr.segment.attributes.u16);
    printf("GDTR =         %016" PRIx64 ":%04x\n", gdtr.table.base, gdtr.table.limit);
    printf("IDTR =         %016" PRIx64 ":%04x\n", idtr.table.base, idtr.table.limit);
    printf("RFLAGS = %016" PRIx64 "", rflags.u64); printRFLAGSBits(rflags.u64); printf("\n");
    printf("EFER = %016" PRIx64, efer.u64); printEFERBits(efer.u64); printf("\n");

    printf(" CR2 = %016" PRIx64 "   CR0 = %016" PRIx64 "", cr2.u64, cr0.u64); printCR0Bits(cr0.u64); printf("\n");
    printf(" CR3 = %016" PRIx64 "   CR4 = %016" PRIx64 "", cr3.u64, cr4.u64); printCR4Bits(cr4.u64); printf("\n");
    printf(" DR0 = %016" PRIx64 "   CR8 = ", dr0.u64);
    if (extendedRegs.AnyOf(ExtendedControlRegister::CR8)) {
        printf("%016" PRIx64 "", cr8.u64); printCR8Bits(cr8.u64); printf("\n");
    }
    else {
        printf("................\n");
    }
    printf(" DR1 = %016" PRIx64 "  XCR0 = ", dr1.u64);
    if (extendedRegs.AnyOf(ExtendedControlRegister::XCR0)) {
        printf("%016" PRIx64 "", xcr0.u64); printXCR0Bits(xcr0.u64); printf("\n");
    }
    else {
        printf("................\n");
    }
    printf(" DR2 = %016" PRIx64 "   DR6 = %016" PRIx64 "", dr2.u64, dr6.u64); printDR6Bits(dr6.u64); printf("\n");
    printf(" DR3 = %016" PRIx64 "   DR7 = %016" PRIx64 "", dr3.u64, dr7.u64); printDR7Bits(dr7.u64); printf("\n");
}

void printFPRegs(VirtualProcessor& vp) noexcept {
    FPUControl fpuCtl;
    auto status = vp.GetFPUControl(fpuCtl);
    if (status != VPOperationStatus::OK) {
        printf("Failed to retrieve FPU control registers\n");
        return;
    }

    Reg regs[] = {
        Reg::ST0, Reg::ST1, Reg::ST2, Reg::ST3, Reg::ST4, Reg::ST5, Reg::ST6, Reg::ST7,
        Reg::MM0, Reg::MM1, Reg::MM2, Reg::MM3, Reg::MM4, Reg::MM5, Reg::MM6, Reg::MM7,
    };
    RegValue values[array_size(regs)];
    
    status = vp.RegRead(regs, values, array_size(regs));
    if (status != VPOperationStatus::OK) {
        printf("Failed to retrieve FPU and MMX registers\n");
        return;
    }
    
    printf("FPU.CW = %04x   FPU.SW = %04x   FPU.TW = %04x   FPU.OP = %04x\n", fpuCtl.cw, fpuCtl.sw, fpuCtl.tw, fpuCtl.op);
    printf("FPU.CS:IP = %04x:%08x\n", fpuCtl.cs, fpuCtl.ip);
    printf("FPU.DS:DP = %04x:%08x\n", fpuCtl.ds, fpuCtl.dp);
    for (int i = 0; i < 8; i++) {
        printf("ST(%d) = %016" PRIx64 " %04x\n", i, values[i].st.significand, values[i].st.exponentSign);
    }
    
    const RegValue *mmValues = &values[8];
    for (int i = 0; i < 8; i++) {
        printf("MM%d = %016" PRIx64 "\n", i, mmValues[i].mm.i64[0]);
    }
}

void printSSERegs(VirtualProcessor& vp) noexcept {
    MXCSR mxcsr, mxcsrMask;
    auto status = vp.GetMXCSR(mxcsr);
    if (status != VPOperationStatus::OK) {
        printf("Failed to retrieve MMX control/status registers\n");
    }

    const auto extCRs = BitmaskEnum(vp.GetVirtualMachine().GetPlatform().GetFeatures().extendedControlRegisters);
    if (extCRs.AnyOf(ExtendedControlRegister::MXCSRMask)) {
        status = vp.GetMXCSRMask(mxcsrMask);
        if (status != VPOperationStatus::OK) {
            printf("Failed to retrieve MXCSR mask\n");
        }
    }

    printf("MXCSR      = %08x\n", mxcsr.u32);
    if (extCRs.AnyOf(ExtendedControlRegister::MXCSRMask)) {
        printf("MXCSR_MASK = %08x\n", mxcsrMask.u32);
    }

    auto caps = vp.GetVirtualMachine().GetPlatform().GetFeatures();
    uint8_t numXMM = 0;
    const auto fpExts = BitmaskEnum(caps.floatingPointExtensions);
    if (fpExts.AnyOf(FloatingPointExtension::SSE2)) {
        numXMM = 8;
    }
    if (fpExts.AnyOf(FloatingPointExtension::VEX)) {
        numXMM = 16;
    }
    if (fpExts.AnyOf(FloatingPointExtension::EVEX)) {
        numXMM = 32;
    }
    for (uint8_t i = 0; i < numXMM; i++) {
        RegValue value;
        status = vp.RegRead(RegAdd(Reg::XMM0, i), value);
        if (status != VPOperationStatus::OK) {
            printf("Failed to read register XMM%u\n", i);
            continue;
        }

        const auto& v = value.xmm;
        printf("XMM%-2u = %016" PRIx64 "  %016" PRIx64 "\n", i, v.i64[0], v.i64[1]);
        printf("        %lf  %lf\n", v.f64[0], v.f64[1]);
    }

    uint8_t numYMM = 0;
    if (fpExts.AnyOf(FloatingPointExtension::AVX)) {
        numYMM = 8;
    }
    if (fpExts.AnyOf(FloatingPointExtension::VEX)) {
        numYMM = 16;
    }
    if (fpExts.AnyOf(FloatingPointExtension::EVEX)) {
        numYMM = 32;
    }
    for (uint8_t i = 0; i < numYMM; i++) {
        RegValue value;
        status = vp.RegRead(RegAdd(Reg::YMM0, i), value);
        if (status != VPOperationStatus::OK) {
            printf("Failed to read register YMM%u\n", i);
            continue;
        }

        const auto& v = value.ymm;
        printf("YMM%-2u = %016" PRIx64 "  %016" PRIx64 "  %016" PRIx64 "  %016" PRIx64 "\n", i, v.i64[0], v.i64[1], v.i64[2], v.i64[3]);
        printf("        %lf  %lf  %lf  %lf\n", v.f64[0], v.f64[1], v.f64[2], v.f64[3]);
    }

    uint8_t numZMM = 0;
    if (fpExts.AnyOf(FloatingPointExtension::AVX512)) {
        numZMM = 8;
    }
    if (fpExts.AnyOf(FloatingPointExtension::VEX)) {
        numZMM = 16;
    }
    if (fpExts.AnyOf((FloatingPointExtension::EVEX | FloatingPointExtension::MVEX))) {
        numZMM = 32;
    }
    for (uint8_t i = 0; i < numZMM; i++) {
        RegValue value;
        status = vp.RegRead(RegAdd(Reg::ZMM0, i), value);
        if (status != VPOperationStatus::OK) {
            printf("Failed to read register ZMM%u\n", i);
            continue;
        }

        const auto& v = value.zmm;
        printf("ZMM%-2u = %016" PRIx64 "  %016" PRIx64 "  %016" PRIx64 "  %016" PRIx64 "  %016" PRIx64 "  %016" PRIx64 "  %016" PRIx64 "  %016" PRIx64 "\n", i, v.i64[0], v.i64[1], v.i64[2], v.i64[3], v.i64[4], v.i64[5], v.i64[6], v.i64[7]);
        printf("        %lf  %lf  %lf  %lf  %lf  %lf  %lf  %lf\n", v.f64[0], v.f64[1], v.f64[2], v.f64[3], v.f64[4], v.f64[5], v.f64[6], v.f64[7]);
    }
}

void printDirtyBitmap(VirtualMachine& vm, uint64_t baseAddress, uint64_t numPages) noexcept {
    if (!vm.GetPlatform().GetFeatures().dirtyPageTracking) {
        printf("Dirty page tracking not supported by the hypervisor\n\n");
    }
    if (numPages == 0) {
        return;
    }

    const size_t bitmapSize = (numPages - 1) / sizeof(uint64_t) + 1;
    uint64_t *bitmap = (uint64_t *)alignedAlloc(bitmapSize * sizeof(uint64_t));
    memset(bitmap, 0, bitmapSize * sizeof(uint64_t));
    const auto dptStatus = vm.QueryDirtyPages(baseAddress, numPages * PAGE_SIZE, bitmap, bitmapSize * sizeof(uint64_t));
    if (dptStatus == DirtyPageTrackingStatus::OK) {
        printf("Dirty pages:\n");
        uint64_t pageNum = 0;
        for (size_t i = 0; i < bitmapSize; i++) {
            if (bitmap[i] == 0) {
                continue;
            }
            for (uint8_t bit = 0; bit < 64; bit++) {
                if (bitmap[i] & (1ull << bit)) {
                    printf("  0x%" PRIx64 "\n", pageNum * PAGE_SIZE);
                }
                if (++pageNum > numPages) {
                    goto done;
                }
            }
        }
        done:
        printf("\n");
    }
    alignedFree(bitmap);
}

void printAddressTranslation(VirtualProcessor& vp, const uint64_t addr) noexcept {
    printf("  0x%" PRIx64 " -> ", addr);
    uint64_t paddr;
    if (vp.LinearToPhysical(addr, &paddr)) {
        printf("0x%" PRIx64 "\n", paddr);
    }
    else {
        printf("<invalid>\n");
    }
}

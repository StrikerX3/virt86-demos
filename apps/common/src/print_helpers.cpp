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

void printMemoryMappingStatus(virt86::MemoryMappingStatus status) noexcept {
    switch (status) {
    case MemoryMappingStatus::OK: printf("succeeded\n"); break;
    case MemoryMappingStatus::Unsupported: printf("failed: unsupported operation\n"); break;
    case MemoryMappingStatus::MisalignedHostMemory: printf("failed: memory host block is misaligned\n"); break;
    case MemoryMappingStatus::MisalignedAddress: printf("failed: base address is misaligned\n"); break;
    case MemoryMappingStatus::MisalignedSize: printf("failed: size is misaligned\n"); break;
    case MemoryMappingStatus::EmptyRange: printf("failed: size is zero\n"); break;
    case MemoryMappingStatus::AlreadyAllocated: printf("failed: host memory block is already allocated\n"); break;
    case MemoryMappingStatus::InvalidFlags: printf("failed: invalid flags supplied\n"); break;
    case MemoryMappingStatus::Failed: printf("failed\n"); break;
    case MemoryMappingStatus::OutOfBounds: printf("out of bounds\n"); break;
    default: printf("failed: unhandled reason (%d)\n", static_cast<int>(status)); break;
    }
}

void printFPExts(FloatingPointExtension fpExts) noexcept {
	auto bmFpExts = BitmaskEnum(fpExts);
	if (!bmFpExts) printf(" None");
	else {
		if (bmFpExts.AnyOf(FloatingPointExtension::MMX)) printf(" MMX");
		if (bmFpExts.AnyOf(FloatingPointExtension::SSE)) printf(" SSE");
		if (bmFpExts.AnyOf(FloatingPointExtension::SSE2)) printf(" SSE2");
		if (bmFpExts.AnyOf(FloatingPointExtension::SSE3)) printf(" SSE3");
		if (bmFpExts.AnyOf(FloatingPointExtension::SSSE3)) printf(" SSSE3");
		if (bmFpExts.AnyOf(FloatingPointExtension::SSE4_1)) printf(" SSE4.1");
		if (bmFpExts.AnyOf(FloatingPointExtension::SSE4_2)) printf(" SSE4.2");
        if (bmFpExts.AnyOf(FloatingPointExtension::SSE4a)) printf(" SSE4a");
        if (bmFpExts.AnyOf(FloatingPointExtension::XOP)) printf(" XOP");
        if (bmFpExts.AnyOf(FloatingPointExtension::F16C)) printf(" F16C");
        if (bmFpExts.AnyOf(FloatingPointExtension::FMA4)) printf(" FMA4");
        if (bmFpExts.AnyOf(FloatingPointExtension::AVX)) printf(" AVX");
        if (bmFpExts.AnyOf(FloatingPointExtension::FMA3)) printf(" FMA3");
		if (bmFpExts.AnyOf(FloatingPointExtension::AVX2)) printf(" AVX2");
		if (bmFpExts.AnyOf(FloatingPointExtension::AVX512F)) {
			printf(" AVX-512[F");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512DQ)) printf(" DQ");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512IFMA)) printf(" IFMA");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512PF)) printf(" PF");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512ER)) printf(" ER");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512CD)) printf(" CD");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512BW)) printf(" BW");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512VL)) printf(" VL");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512VBMI)) printf(" VBMI");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512VBMI2)) printf(" VBMI2");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512GFNI)) printf(" GFNI");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512VAES)) printf(" VAES");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512VNNI)) printf(" VNNI");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512BITALG)) printf(" BITALG");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512VPOPCNTDQ)) printf(" VPOPCNTDQ");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512QVNNIW)) printf(" QVNNIW");
			if (bmFpExts.AnyOf(FloatingPointExtension::AVX512QFMA)) printf(" QFMA");
			printf("]");
		}
	}
}

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

enum class CPUMode {
    Unknown,
    
    RealAddress,
    Virtual8086,
    Protected,
    IA32e,
};

CPUMode getCPUMode(VirtualProcessor& vp) noexcept {
    RegValue cr0, rflags, efer;
    vp.RegRead(Reg::CR0, cr0);
    vp.RegRead(Reg::RFLAGS, rflags);
    vp.RegRead(Reg::EFER, efer);

    bool cr0_pe = (cr0.u64 & CR0_PE) != 0;
    bool rflags_vm = (rflags.u64 & RFLAGS_VM) != 0;
    bool efer_lma = (efer.u64 & EFER_LMA) != 0;

    if (!cr0_pe) {
        return CPUMode::RealAddress;
    }
    if (rflags_vm) {
        return CPUMode::Virtual8086;
    }
    else if (efer_lma) {
        return CPUMode::IA32e;
    }
    return CPUMode::Protected;
}

enum class PagingMode {
    Unknown,
    Invalid,

    None,
    NoneLME,
    NonePAE,
    NonePAEandLME,
    ThirtyTwoBit,
    PAE,
    FourLevel,
};

PagingMode getPagingMode(VirtualProcessor& vp) noexcept {
    RegValue cr0, cr4, efer;
    vp.RegRead(Reg::CR0, cr0);
    vp.RegRead(Reg::CR4, cr4);
    vp.RegRead(Reg::EFER, efer);

    bool cr0_pg = (cr0.u64 & CR0_PG) != 0;
    bool cr4_pae = (cr4.u64 & CR4_PAE) != 0;
    bool efer_lme = (efer.u64 & EFER_LME) != 0;

    uint8_t pagingModeBits = 0
        | (cr0_pg ? (1 << 2) : 0)
        | (cr4_pae ? (1 << 1) : 0)
        | (efer_lme ? (1 << 0) : 0);

    switch (pagingModeBits) {
    case 0b000: return PagingMode::None;
    case 0b001: return PagingMode::NoneLME;
    case 0b010: return PagingMode::NonePAE;
    case 0b011: return PagingMode::NonePAEandLME;
    case 0b100: return PagingMode::ThirtyTwoBit;
    case 0b101: return PagingMode::Invalid;
    case 0b110: return PagingMode::PAE;
    case 0b111: return PagingMode::FourLevel;
    default: return PagingMode::Unknown;
    }
}

enum class SegmentSize {
    Invalid,

    _16,
    _32,
    _64,
};

SegmentSize getSegmentSize(VirtualProcessor& vp, Reg segmentReg) noexcept {
    size_t regOffset = RegOffset<size_t>(Reg::CS, segmentReg);
    size_t maxOffset = RegOffset<size_t>(Reg::CS, Reg::TR);
    if (regOffset > maxOffset) {
        return SegmentSize::Invalid;
    }

    RegValue value;
    vp.RegRead(segmentReg, value);

    CPUMode cpuMode = getCPUMode(vp);

    if (cpuMode == CPUMode::IA32e && value.segment.attributes.longMode) {
        return SegmentSize::_64;
    }
    if (value.segment.attributes.defaultSize) {
        return SegmentSize::_32;
    }
    return SegmentSize::_16;
}

void printSeg(VirtualProcessor& vp, Reg seg) noexcept {
	CPUMode mode = getCPUMode(vp);
	SegmentSize size = getSegmentSize(vp, seg);
	RegValue value;
	vp.RegRead(seg, value);

	// In IA-32e mode:
	// - Limit is ignored for CS, SS, DS, ES, FS and GS (effectively giving access to the entire memory)
	// - CS, SS, DS, ES all have base addresses of 0
	// - FS and GS have their base addresses stored in MSRs
	// - LDT and TSS entries are extended to 16 bytes to accomodate a 64-bit base address

	if (mode == CPUMode::IA32e) {
		if (seg == Reg::LDTR || seg == Reg::TR) {
			printf("%04" PRIx16 " -> %016" PRIx64 ":%08" PRIx32 " [%04" PRIx16 "] ", value.segment.selector, value.segment.base, value.segment.limit, value.segment.attributes.u16);
		}
		else {
			printf("%04" PRIx16 " -> %016" PRIx64 "          [%04" PRIx16 "] ", value.segment.selector, value.segment.base, value.segment.attributes.u16);
		}
	}
	else {
		switch (size) {
		case SegmentSize::_16: printf("%04" PRIx16 " -> %08" PRIx32 ":%04" PRIx16 "     [%04" PRIx16 "] ", value.segment.selector, (uint32_t)value.segment.base, (uint16_t)value.segment.limit, value.segment.attributes.u16); break;
		case SegmentSize::_32: printf("%04" PRIx16 " -> %08" PRIx32 ":%08" PRIx32 " [%04" PRIx16 "] ", value.segment.selector, (uint32_t)value.segment.base, value.segment.limit, value.segment.attributes.u16); break;
		}
	}

	// Print attributes
	if (value.segment.attributes.present) {
		if (value.segment.attributes.system) {
			if (value.segment.attributes.type & SEG_TYPE_CODE) {
				if (mode == CPUMode::IA32e && value.segment.attributes.longMode) printf("64-bit code");
				else if (value.segment.attributes.defaultSize) printf("32-bit code");
				else printf("16-bit code");
			}
			else {
				if (mode == CPUMode::IA32e) printf("64-bit data");
				else if (value.segment.attributes.defaultSize) printf("32-bit data");
				else printf("16-bit data");
			}
		}
		else {
			if (mode == CPUMode::IA32e) {
				switch (value.segment.attributes.type) {
				case 0b0010: printf("LDT"); break;
				case 0b1001: printf("64-bit TSS (available)"); break;
				case 0b1011: printf("64-bit TSS (busy)"); break;
				case 0b1100: printf("64-bit call gate"); break;
				case 0b1110: printf("64-bit interrupt gate"); break;
				case 0b1111: printf("64-bit trap gate"); break;
				default: printf("Reserved"); break;
				}
			}
			else {
				switch (value.segment.attributes.type) {
				case 0b0010: printf("LDT"); break;
				case 0b0001: printf("16-bit TSS (available)"); break;
				case 0b0011: printf("16-bit TSS (busy)"); break;
				case 0b0100: printf("16-bit call gate"); break;
				case 0b0110: printf("16-bit interrupt gate"); break;
				case 0b0111: printf("16-bit trap gate"); break;
				case 0b0101: printf("Task gate"); break;
				case 0b1001: printf("32-bit TSS (available)"); break;
				case 0b1011: printf("32-bit TSS (busy)"); break;
				case 0b1100: printf("32-bit call gate"); break;
				case 0b1110: printf("32-bit interrupt gate"); break;
				case 0b1111: printf("32-bit trap gate"); break;
				default: printf("Reserved"); break;
				}
			}
		}
		
		printf(" (");
		printf((value.segment.attributes.granularity) ? "G=page" : "G=byte");
		printf(" DPL=%u", value.segment.attributes.privilegeLevel);
		if (value.segment.attributes.system) {
			if (value.segment.attributes.type & SEG_TYPE_CODE) {
				if (value.segment.attributes.type & SEG_TYPE_READABLE) printf(" R-X"); else printf(" --X");
				if (value.segment.attributes.type & SEG_TYPE_ACCESSED) printf("A"); else printf("-");
				if (value.segment.attributes.type & SEG_TYPE_CONFORMING) printf(" conforming");
			}
			else {
				if (value.segment.attributes.type & SEG_TYPE_WRITABLE) printf(" RW-"); else printf(" R--");
				if (value.segment.attributes.type & SEG_TYPE_ACCESSED) printf("A"); else printf("-");
				if (value.segment.attributes.type & SEG_TYPE_EXPANDDOWN) printf(" expand-down");
			}
		}
		if (value.segment.attributes.available) printf(" AVL");
		printf(")");
	}
}

void printTable(VirtualProcessor& vp, Reg table) noexcept {
	CPUMode mode = getCPUMode(vp);
	RegValue value;
	vp.RegRead(table, value);

	if (mode == CPUMode::IA32e) {
		printf("%016" PRIx64 ":%04" PRIx16, value.table.base, value.table.limit);
	}
	else {
		printf("%08" PRIx32 ":%04" PRIx16, (uint32_t)value.table.base, value.table.limit);
	}
}

#define READREG(code, name) bool has_##name; RegValue name; has_##name = vp.RegRead(code, name) == VPOperationStatus::OK;
void printSegAndTableRegs(VirtualProcessor& vp) noexcept {
	READREG(Reg::CS, cs);
	READREG(Reg::SS, ss);
	READREG(Reg::DS, ds);
	READREG(Reg::ES, es);
	READREG(Reg::FS, fs);
	READREG(Reg::GS, gs);
	READREG(Reg::TR, tr);
	READREG(Reg::LDTR, ldtr);
	READREG(Reg::GDTR, gdtr);
	READREG(Reg::IDTR, idtr);
	
	printf("  CS = "); printSeg(vp, Reg::CS); printf("\n");
	printf("  SS = "); printSeg(vp, Reg::SS); printf("\n");
	printf("  DS = "); printSeg(vp, Reg::DS); printf("\n");
	printf("  ES = "); printSeg(vp, Reg::ES); printf("\n");
	printf("  FS = "); printSeg(vp, Reg::FS); printf("\n");
	printf("  GS = "); printSeg(vp, Reg::GS); printf("\n");
	printf("  TR = "); printSeg(vp, Reg::TR); printf("\n");
	printf("LDTR = "); printSeg(vp, Reg::LDTR); printf("\n");
	printf("GDTR =         "); printTable(vp, Reg::GDTR); printf("\n");
	printf("IDTR =         "); printTable(vp, Reg::IDTR); printf("\n");
}

void printControlAndDebugRegs(VirtualProcessor& vp) noexcept {
	READREG(Reg::EFER, efer);
	READREG(Reg::CR2, cr2); READREG(Reg::CR0, cr0);
	READREG(Reg::CR3, cr3); READREG(Reg::CR4, cr4);
	READREG(Reg::DR0, dr0); READREG(Reg::CR8, cr8);
	READREG(Reg::DR1, dr1); READREG(Reg::XCR0, xcr0);
	READREG(Reg::DR2, dr2); READREG(Reg::DR6, dr6);
	READREG(Reg::DR3, dr3); READREG(Reg::DR7, dr7);

	CPUMode mode = getCPUMode(vp);

	const auto extendedRegs = BitmaskEnum(vp.GetVirtualMachine().GetPlatform().GetFeatures().extendedControlRegisters);

	printf("EFER = %016" PRIx64, efer.u64); printEFERBits(efer.u64); printf("\n");
	if (mode == CPUMode::IA32e) {
		printf(" CR2 = %016" PRIx64 "   CR0 = %016" PRIx64, cr2.u64, cr0.u64); printCR0Bits(cr0.u64); printf("\n");
		printf(" CR3 = %016" PRIx64 "   CR4 = %016" PRIx64, cr3.u64, cr4.u64); printCR4Bits(cr4.u64); printf("\n");
		printf(" DR0 = %016" PRIx64 "   CR8 = ", dr0.u64);
		if (extendedRegs.AnyOf(ExtendedControlRegister::CR8) && has_cr8) {
			printf("%016" PRIx64, cr8.u64); printCR8Bits(cr8.u64); printf("\n");
		}
		else {
			printf("................\n");
		}
		printf(" DR1 = %016" PRIx64 "  XCR0 = ", dr1.u64);
		if (extendedRegs.AnyOf(ExtendedControlRegister::XCR0) && has_xcr0) {
			printf("%016" PRIx64, xcr0.u64); printXCR0Bits(xcr0.u64); printf("\n");
		}
		else {
			printf("................\n");
		}
		printf(" DR2 = %016" PRIx64 "   DR6 = %016" PRIx64, dr2.u64, dr6.u64); printDR6Bits(dr6.u64); printf("\n");
		printf(" DR3 = %016" PRIx64 "   DR7 = %016" PRIx64, dr3.u64, dr7.u64); printDR7Bits(dr7.u64); printf("\n");
	}
	else {
		printf(" CR2 = %08" PRIx32 "   CR0 = %08" PRIx32, cr2.u32, cr0.u32); printCR0Bits(cr0.u32); printf("\n");
		printf(" CR3 = %08" PRIx32 "   CR4 = %08" PRIx32, cr3.u32, cr4.u32); printCR4Bits(cr4.u32); printf("\n");
		printf(" DR0 = %08" PRIx32 "\n", dr0.u32);
		printf(" DR1 = %08" PRIx32 "  XCR0 = ", dr1.u32);
		if (extendedRegs.AnyOf(ExtendedControlRegister::XCR0) && has_xcr0) {
			printf("%016" PRIx64, xcr0.u64); printXCR0Bits(xcr0.u64); printf("\n");
		}
		else {
			printf("................\n");
		}
		printf(" DR2 = %08" PRIx32 "   DR6 = %08" PRIx32, dr2.u32, dr6.u32); printDR6Bits(dr6.u32); printf("\n");
		printf(" DR3 = %08" PRIx32 "   DR7 = %08" PRIx32, dr3.u32, dr7.u32); printDR7Bits(dr7.u32); printf("\n");
	}
}

void printRegs16(VirtualProcessor& vp) noexcept {
	READREG(Reg::EAX, eax); READREG(Reg::ECX, ecx); READREG(Reg::EDX, edx); READREG(Reg::EBX, ebx);
	READREG(Reg::ESP, esp); READREG(Reg::EBP, ebp); READREG(Reg::ESI, esi); READREG(Reg::EDI, edi);
	READREG(Reg::IP, ip);
	READREG(Reg::EFLAGS, eflags);

	printf(" EAX = %08" PRIx32 "   ECX = %08" PRIx32 "   EDX = %08" PRIx32 "   EBX = %08" PRIx32 "\n", eax.u32, ecx.u32, edx.u32, ebx.u32);
	printf(" ESP = %08" PRIx32 "   EBP = %08" PRIx32 "   ESI = %08" PRIx32 "   EDI = %08" PRIx32 "\n", esp.u32, ebp.u32, esi.u32, edi.u32);
	printf("  IP = %04" PRIx16 "\n", ip.u16);
	printSegAndTableRegs(vp);
	printf("EFLAGS = %08" PRIx32, eflags.u32); printRFLAGSBits(eflags.u32); printf("\n");
	printControlAndDebugRegs(vp);
}

void printRegs32(VirtualProcessor& vp) noexcept {
    READREG(Reg::EAX, eax); READREG(Reg::ECX, ecx); READREG(Reg::EDX, edx); READREG(Reg::EBX, ebx);
    READREG(Reg::ESP, esp); READREG(Reg::EBP, ebp); READREG(Reg::ESI, esi); READREG(Reg::EDI, edi);
    READREG(Reg::EIP, eip);
    READREG(Reg::EFLAGS, eflags);

    printf(" EAX = %08" PRIx32 "   ECX = %08" PRIx32 "   EDX = %08" PRIx32 "   EBX = %08" PRIx32 "\n", eax.u32, ecx.u32, edx.u32, ebx.u32);
    printf(" ESP = %08" PRIx32 "   EBP = %08" PRIx32 "   ESI = %08" PRIx32 "   EDI = %08" PRIx32 "\n", esp.u32, ebp.u32, esi.u32, edi.u32);
    printf(" EIP = %08" PRIx32 "\n", eip.u32);
	printSegAndTableRegs(vp);
    printf("EFLAGS = %08" PRIx32, eflags.u32); printRFLAGSBits(eflags.u32); printf("\n");
	printControlAndDebugRegs(vp);
}

void printRegs64(VirtualProcessor& vp) noexcept {
    READREG(Reg::RAX, rax); READREG(Reg::RCX, rcx); READREG(Reg::RDX, rdx); READREG(Reg::RBX, rbx);
    READREG(Reg::RSP, rsp); READREG(Reg::RBP, rbp); READREG(Reg::RSI, rsi); READREG(Reg::RDI, rdi);
    READREG(Reg::R8, r8); READREG(Reg::R9, r9); READREG(Reg::R10, r10); READREG(Reg::R11, r11);
    READREG(Reg::R12, r12); READREG(Reg::R13, r13); READREG(Reg::R14, r14); READREG(Reg::R15, r15);
    READREG(Reg::RIP, rip);
    READREG(Reg::RFLAGS, rflags);

    printf(" RAX = %016" PRIx64 "   RCX = %016" PRIx64 "   RDX = %016" PRIx64 "   RBX = %016" PRIx64 "\n", rax.u64, rcx.u64, rdx.u64, rbx.u64);
    printf(" RSP = %016" PRIx64 "   RBP = %016" PRIx64 "   RSI = %016" PRIx64 "   RDI = %016" PRIx64 "\n", rsp.u64, rbp.u64, rsi.u64, rdi.u64);
    printf("  R8 = %016" PRIx64 "    R9 = %016" PRIx64 "   R10 = %016" PRIx64 "   R11 = %016" PRIx64 "\n", r8.u64, r9.u64, r10.u64, r11.u64);
    printf(" R12 = %016" PRIx64 "   R13 = %016" PRIx64 "   R14 = %016" PRIx64 "   R15 = %016" PRIx64 "\n", r12.u64, r13.u64, r14.u64, r15.u64);
    printf(" RIP = %016" PRIx64 "\n", rip.u64);
	printSegAndTableRegs(vp);
    printf("RFLAGS = %016" PRIx64, rflags.u64); printRFLAGSBits(rflags.u64); printf("\n");
	printControlAndDebugRegs(vp);
}
#undef READREG

void printRegs(VirtualProcessor& vp) noexcept {
    // Print CPU mode, paging mode and code segment size
    CPUMode cpuMode = getCPUMode(vp);
    PagingMode pagingMode = getPagingMode(vp);
    SegmentSize segmentSize = getSegmentSize(vp, Reg::CS);

    switch (cpuMode) {
    case CPUMode::RealAddress: printf("Real-address mode"); break;
    case CPUMode::Virtual8086: printf("Virtual-8086 mode"); break;
    case CPUMode::Protected: printf("Protected mode"); break;
    case CPUMode::IA32e: printf("IA-32e mode"); break;
    }
    printf(", ");

    switch (pagingMode) {
    case PagingMode::None: printf("no paging"); break;
    case PagingMode::NoneLME: printf("no paging (LME enabled)"); break;
    case PagingMode::NonePAE: printf("no paging (PAE enabled)"); break;
    case PagingMode::NonePAEandLME: printf("no paging (PAE and LME enabled)"); break;
    case PagingMode::ThirtyTwoBit: printf("32-bit paging"); break;
    case PagingMode::Invalid: printf("*invalid*"); break;
    case PagingMode::PAE: printf("PAE paging"); break;
    case PagingMode::FourLevel: printf("4-level paging"); break;
    }
    printf(", ");

    switch (segmentSize) {
    case SegmentSize::_16: printf("16-bit code"); break;
    case SegmentSize::_32: printf("32-bit code"); break;
    case SegmentSize::_64: printf("64-bit code"); break;
    }
    printf("\n");

    // Print registers according to segment size
    switch (segmentSize) {
    case SegmentSize::_16: printRegs16(vp); break;
    case SegmentSize::_32: printRegs32(vp); break;
    case SegmentSize::_64: printRegs64(vp); break;
    }
}

void printFPUControlRegs(VirtualProcessor& vp) noexcept {
    FPUControl fpuCtl;
    auto status = vp.GetFPUControl(fpuCtl);
    if (status != VPOperationStatus::OK) {
        printf("Failed to retrieve FPU control registers\n");
        return;
    }

    printf("FPU.CW = %04x   FPU.SW = %04x   FPU.TW = %04x   FPU.OP = %04x\n", fpuCtl.cw, fpuCtl.sw, fpuCtl.tw, fpuCtl.op);
    printf("FPU.CS:IP = %04x:%08x\n", fpuCtl.cs, fpuCtl.ip);
    printf("FPU.DS:DP = %04x:%08x\n", fpuCtl.ds, fpuCtl.dp);
}

void printSTRegs(VirtualProcessor& vp) noexcept {
    Reg regs[] = {
        Reg::ST0, Reg::ST1, Reg::ST2, Reg::ST3, Reg::ST4, Reg::ST5, Reg::ST6, Reg::ST7,
    };
    RegValue values[array_size(regs)];

    auto status = vp.RegRead(regs, values, array_size(regs));
    if (status != VPOperationStatus::OK) {
        printf("Failed to retrieve FPU registers\n");
        return;
    }

    for (int i = 0; i < 8; i++) {
        printf("ST(%d) = %016" PRIx64 " %04x\n", i, values[i].st.significand, values[i].st.exponentSign);
    }
}

void printMMRegs(VirtualProcessor& vp, MMBits bits) noexcept {
    Reg regs[] = {
        Reg::MM0, Reg::MM1, Reg::MM2, Reg::MM3, Reg::MM4, Reg::MM5, Reg::MM6, Reg::MM7,
    };
    RegValue values[array_size(regs)];

    auto status = vp.RegRead(regs, values, array_size(regs));
    if (status != VPOperationStatus::OK) {
        printf("Failed to retrieve MMX registers\n");
        return;
    }
    
    for (int i = 0; i < 8; i++) {
        printf(" MM%d =", i);
        switch (bits) {
        case MMBits::_8:
            for (int j = 7; j >= 0; j--) {
                printf(" %02" PRIx8, values[i].mm.i8[j]);
            }
            break;
        case MMBits::_16:
            for (int j = 3; j >= 0; j--) {
                printf(" %04" PRIx16, values[i].mm.i16[j]);
            }
            break;
        case MMBits::_32:
            for (int j = 1; j >= 0; j--) {
                printf(" %08" PRIx32, values[i].mm.i32[j]);
            }
            break;
        case MMBits::_64:
            printf(" %016" PRIx64, values[i].mm.i64[0]);
            break;
        }
        printf("\n");
    }
}

void printMXCSRRegs(VirtualProcessor& vp) noexcept {
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
}

void printXMMRegs(VirtualProcessor& vp, MMBits bits) noexcept {
    auto cpuMode = getCPUMode(vp);
    const uint8_t maxMMRegs = (cpuMode == CPUMode::IA32e) ? 32 : 8;

    for (uint8_t i = 0; i < maxMMRegs; i++) {
        RegValue value;
        auto status = vp.RegRead(RegAdd(Reg::XMM0, i), value);
        if (status != VPOperationStatus::OK) {
            break;
        }

        const auto& v = value.xmm;
        printf("XMM%-2u =", i);
        switch (bits) {
        case MMBits::_8:
            for (int j = 15; j >= 0; j--) {
                printf(" %02" PRIx8, v.i8[j]);
            }
            printf("\n");
            break;
        case MMBits::_16:
            for (int j = 7; j >= 0; j--) {
                printf("  %04" PRIx16, v.i16[j]);
            }
            printf("\n");
            break;
        case MMBits::_32:
            for (int j = 3; j >= 0; j--) {
                printf("  %08" PRIx32, v.i32[j]);
            }
            printf("\n");
            printf("       ");
            for (int j = 3; j >= 0; j--) {
                printf("  %f", v.f32[j]);
            }
            printf("\n");
            break;
        case MMBits::_64:
            for (int j = 1; j >= 0; j--) {
                printf("  %016" PRIx64, v.i64[j]);
            }
            printf("\n");
            printf("       ");
            for (int j = 1; j >= 0; j--) {
                printf("  %lf", v.f64[j]);
            }
            printf("\n");
            break;
        }
    }
}

void printYMMRegs(VirtualProcessor& vp, MMBits bits) noexcept {
    auto cpuMode = getCPUMode(vp);
    const uint8_t maxMMRegs = (cpuMode == CPUMode::IA32e) ? 32 : 8;

    for (uint8_t i = 0; i < maxMMRegs; i++) {
        RegValue value;
        auto status = vp.RegRead(RegAdd(Reg::YMM0, i), value);
        if (status != VPOperationStatus::OK) {
            break;
        }

        const auto& v = value.ymm;
        printf("YMM%-2u =", i);
        switch (bits) {
        case MMBits::_8:
            for (int j = 31; j >= 0; j--) {
                printf(" %02" PRIx8, v.i8[j]);
            }
            printf("\n");
            break;
        case MMBits::_16:
            for (int j = 15; j >= 0; j--) {
                printf("  %04" PRIx16, v.i16[j]);
            }
            printf("\n");
            break;
        case MMBits::_32:
            for (int j = 7; j >= 0; j--) {
                printf("  %08" PRIx32, v.i32[j]);
            }
            printf("\n");
            printf("       ");
            for (int j = 7; j >= 0; j--) {
                printf("  %f", v.f32[j]);
            }
            printf("\n");
            break;
        case MMBits::_64:
            for (int j = 3; j >= 0; j--) {
                printf("  %016" PRIx64, v.i64[j]);
            }
            printf("\n");
            printf("       ");
            for (int j = 3; j >= 0; j--) {
                printf("  %lf", v.f64[j]);
            }
            printf("\n");
            break;
        }
    }
}

void printZMMRegs(VirtualProcessor& vp, MMBits bits) noexcept {
    auto cpuMode = getCPUMode(vp);
    const uint8_t maxMMRegs = (cpuMode == CPUMode::IA32e) ? 32 : 8;

    for (uint8_t i = 0; i < maxMMRegs; i++) {
        RegValue value;
        auto status = vp.RegRead(RegAdd(Reg::ZMM0, i), value);
        if (status != VPOperationStatus::OK) {
            break;
        }

        const auto& v = value.zmm;
        printf("YMM%-2u =", i);
        switch (bits) {
        case MMBits::_8:
            for (int j = 63; j >= 0; j--) {
                printf(" %02" PRIx8, v.i8[j]);
            }
            printf("\n");
            break;
        case MMBits::_16:
            for (int j = 31; j >= 0; j--) {
                printf("  %04" PRIx16, v.i16[j]);
            }
            printf("\n");
            break;
        case MMBits::_32:
            for (int j = 15; j >= 0; j--) {
                printf("  %08" PRIx32, v.i32[j]);
            }
            printf("\n");
            printf("       ");
            for (int j = 15; j >= 0; j--) {
                printf("  %f", v.f32[j]);
            }
            printf("\n");
            break;
        case MMBits::_64:
            for (int j = 7; j >= 0; j--) {
                printf("  %016" PRIx64, v.i64[j]);
            }
            printf("\n");
            printf("       ");
            for (int j = 7; j >= 0; j--) {
                printf("  %lf", v.f64[j]);
            }
            printf("\n");
            break;
        }
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

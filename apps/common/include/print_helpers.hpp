/*
Declares helper functions that print out formatted register values.
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
#pragma once

#include "virt86/virt86.hpp"

#include <cstdint>

enum class MMBits {
    _8, _16, _32, _64
};

void printFPExts(virt86::FloatingPointExtension fpExts) noexcept;
void printRegs(virt86::VirtualProcessor& vp) noexcept;
void printFPUControlRegs(virt86::VirtualProcessor& vp) noexcept;
void printSTRegs(virt86::VirtualProcessor& vp) noexcept;
void printMMRegs(virt86::VirtualProcessor& vp, MMBits bits) noexcept;
void printMXCSRRegs(virt86::VirtualProcessor& vp) noexcept;
void printXMMRegs(virt86::VirtualProcessor& vp, MMBits bits) noexcept;
void printYMMRegs(virt86::VirtualProcessor& vp, MMBits bits) noexcept;
void printZMMRegs(virt86::VirtualProcessor& vp, MMBits bits) noexcept;
void printDirtyBitmap(virt86::VirtualMachine& vm, uint64_t baseAddress, uint64_t numPages) noexcept;
void printAddressTranslation(virt86::VirtualProcessor& vp, const uint64_t addr) noexcept;

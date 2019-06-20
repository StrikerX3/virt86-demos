/*
Defines general utility functions.
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
#include "utils.hpp"

const char *reason_str(virt86::VMExitReason reason) noexcept {
    switch (reason) {
    case virt86::VMExitReason::Normal: return "Normal";
    case virt86::VMExitReason::Cancelled: return "Cancelled";
    case virt86::VMExitReason::Interrupt: return "Interrupt";
    case virt86::VMExitReason::PIO: return "Port I/O";
    case virt86::VMExitReason::MMIO: return "MMIO";
    case virt86::VMExitReason::Step: return "Single stepping";
    case virt86::VMExitReason::SoftwareBreakpoint: return "Software breakpoint";
    case virt86::VMExitReason::HardwareBreakpoint: return "Hardware breakpoint";
    case virt86::VMExitReason::HLT: return "HLT instruction";
    case virt86::VMExitReason::CPUID: return "CPUID instruction";
    case virt86::VMExitReason::MSRAccess: return "MSR access";
    case virt86::VMExitReason::Exception: return "CPU exception";
    case virt86::VMExitReason::Shutdown: return "VM is shutting down";
    case virt86::VMExitReason::Error: return "Hypervisor error";
    case virt86::VMExitReason::Unhandled: return "Unhandled reason";
    default: return "Unknown/unexpected reason";
    }
}

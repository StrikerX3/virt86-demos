# 64-bit guest demo

This application creates a virtual machine where the guest switches into 64-bit long mode and performs a series of 64-bit operations.

The initialization procedure follows the instructions on [Entering Long Mode Directly in the OSDev wiki](https://wiki.osdev.org/Entering_Long_Mode_Directly).

The application also executes several floating point instructions from the MMX, SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, FMA3 and AVX2 extensions, depending on support from the virtualization platform and the host CPU.

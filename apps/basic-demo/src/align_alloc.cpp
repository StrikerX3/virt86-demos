/*
Defines cross-platform functions for allocating and freeing aligned memory.
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
#include "align_alloc.hpp"

#include "virt86/vp/vp.hpp"

#if defined(_WIN32)
#  include <Windows.h>
#elif defined(__linux__)
#  include <stdlib.h>
#elif defined(__APPLE__)
#  include <stdlib.h>
#else
#  error Unsupported platform
#endif

uint8_t *alignedAlloc(const size_t size) {
#if defined(_WIN32)
    LPVOID mem = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
    if (mem == NULL) {
        return NULL;
    }
    return (uint8_t *)VirtualAlloc(mem, size, MEM_COMMIT, PAGE_READWRITE);
#elif defined(__linux__)
    return (uint8_t *)aligned_alloc(PAGE_SIZE, size);
#elif defined(__APPLE__)
    // Allocate memory with room to keep track of the original pointer
    void *mem = malloc(size + (PAGE_SIZE - 1) + sizeof(void*));

    // Get aligned address
    uint8_t *alignedMem = ((uint8_t *)mem + sizeof(void*));
    alignedMem += PAGE_SIZE - ((uintptr_t)alignedMem & (PAGE_SIZE - 1));

    // Write pointer to original memory block just before the aligned memory block
    ((void **)alignedMem)[-1] = mem;

    return alignedMem;
#endif
}

bool alignedFree(void *memory) {
#if defined(_WIN32)
    return VirtualFree(memory, 0, MEM_RELEASE) == TRUE;
#elif defined(__linux__)
    free(memory);
    return true;
#elif defined(__APPLE__)
    free(((void **)memory)[-1]);
    return true;
#endif
}

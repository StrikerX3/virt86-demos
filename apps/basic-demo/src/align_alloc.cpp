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

#if defined(_WIN32)
#  include <Windows.h>
#elif defined(__linux__)
#  include <stdlib.h>
#elif defined(__APPLE__)
#  include <stdlib.h>
#endif

uint8_t *alignedAlloc(const size_t size) {
#if defined(_WIN32)
    LPVOID mem = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
    if (mem == NULL) {
        return NULL;
    }
    return (uint8_t *)VirtualAlloc(mem, size, MEM_COMMIT, PAGE_READWRITE);
#else
    return (uint8_t *)aligned_alloc(PAGE_SIZE, size);
#endif
}

bool alignedFree(void *memory) {
#if defined(_WIN32)
    return VirtualFree(memory, 0, MEM_RELEASE) == TRUE;
#else
    free(memory);
    return true;
#endif
}

/***************************************************************************************[System.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "candy/utils/System.h"

#if defined(_WIN32)
#include <Windows.h>
#include <cassert>
#include <chrono>

static HANDLE processHandle = INVALID_HANDLE_VALUE;

std::chrono::milliseconds Candy::cpuTime(void) {
    if (processHandle == INVALID_HANDLE_VALUE) {
        processHandle = GetCurrentProcess();
    }
    
    FILETIME creationTimeFT;
    FILETIME exitTimeFT;
    FILETIME kernelTimeFT;
    FILETIME userTimeFT;
    
    bool success = (GetProcessTimes(processHandle,
                                    &creationTimeFT,
                                    &exitTimeFT,
                                    &kernelTimeFT,
                                    &userTimeFT) != 0);
    assert(success);
    
    uint64_t userTime = userTimeFT.dwLowDateTime + ((uint64_t)userTimeFT.dwHighDateTime << 32);
    uint64_t kernelTime = kernelTimeFT.dwLowDateTime + ((uint64_t)kernelTimeFT.dwHighDateTime << 32);
    
    // times are given with 100ns resolution
    return std::chrono::milliseconds{(userTime + kernelTime) / (10 * 1000)};
}

#endif

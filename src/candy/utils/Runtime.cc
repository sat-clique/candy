/* Copyright (c) 2017 - 2019 Markus Iser
 
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
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Candy { 

#ifdef _WIN32
    double get_wall_time() {
        LARGE_INTEGER time,freq;
        if (!QueryPerformanceFrequency(&freq)){
            //  Handle error
            return 0;
        }
        if (!QueryPerformanceCounter(&time)){
            //  Handle error
            return 0;
        }
        return (double)time.QuadPart / freq.QuadPart;
    }

    double get_cpu_time() {
        FILETIME a,b,c,d;
        if (GetProcessTimes(GetCurrentProcess(),&a,&b,&c,&d) != 0){
            //  Returns total user time.
            //  Can be tweaked to include kernel times as well.
            return
                (double)(d.dwLowDateTime |
                ((unsigned long long)d.dwHighDateTime << 32)) * 0.0000001;
        } else{
            //  Handle error
            return 0;
        }
    }
#else
#include <time.h>
#include <sys/time.h>
    double get_wall_time() {
        struct timeval time;
        if (gettimeofday(&time, NULL)){
            //  Handle error
            return 0;
        }
        return (double)time.tv_sec + (double)time.tv_usec * .000001;
    }

    unsigned int get_cpu_time() {
        return (unsigned int)((double)clock() / (double)CLOCKS_PER_SEC);
    }
#endif

}
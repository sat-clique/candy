/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef StreamBuffer_h
#define StreamBuffer_h

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <zlib.h>
#include <memory>
#include <assert.h>

namespace Candy {

static const int buffer_size = 1024*1024;

class StreamBuffer {
    gzFile in;
    std::unique_ptr<unsigned char[]> buf;
    int pos;
    int size;
    int offset;

void assureLookahead();

public:
explicit StreamBuffer(gzFile i) :
    in(i), pos(0), size(0), offset(0) {
        buf = std::unique_ptr<unsigned char[]>(new unsigned char[buffer_size]);
        assureLookahead();
}

int readInteger();

int operator *() const {
    return (pos >= size - offset) ? EOF : buf[pos];
}

void skipLine();
void skipWhitespace();
bool skipString(const char* str);

void incPos(unsigned int inc) {
    assert(inc > 0);
    pos += inc;
    assureLookahead();
}

void operator ++() {
    incPos(1);
}

bool eof() {
    return pos >= size - offset;
}

};

}

#endif

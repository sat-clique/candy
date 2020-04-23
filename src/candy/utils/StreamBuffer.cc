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

#include "candy/utils/StreamBuffer.h"
#include "candy/utils/Exceptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <errno.h>
#include <string.h>
#include <limits>

namespace Candy {

    /** Skip until the end of the next newline (+subsequent whitespace) */
    void StreamBuffer::skipLine() {
        while (!eof() && (!isspace(buffer[pos]) || isblank(buffer[pos]))) {
            incPos(1);
        }
        skipWhitespace();
    }

    /** Skip whitespace */
    void StreamBuffer::skipWhitespace() {
        while (!eof() && isspace(buffer[pos])) {
            incPos(1);
        }
    }

    /** Skip given sequence of character (+throw exceptions if input deviates) */
    void StreamBuffer::skipString(const char* str) {
        for (; *str != '\0'; ++str, incPos(1)) {
            if (*str != buffer[pos] || eof()) {
                throw ParserException(std::string("PARSE ERROR! Expected '") + std::string(str) + std::string("' but found ") + std::string(1, buffer[pos]));
            }
        }
    }

    int StreamBuffer::readInteger() {
        if (eof()) throw ParserException(std::string("PARSE ERROR! Unexpected end of file"));

        char* str = buffer + pos;
        char* end = NULL;
                
        errno = 0;
        long number = strtol(str, &end, 10);

        if (errno == ERANGE || 2*abs(number) >= std::numeric_limits<uint32_t>::max()) {
            throw ParserException(std::string("PARSE ERROR! Variable out of range: ") + std::to_string(number));
        }
        else if (errno != 0) {
            // Had ERRNO=25 ('Not a typewriter') when acceptance tests are run from gTest
            // Had ERRNO=29 ('Illegal seek') when reading from stdin
            throw ParserException(std::string("PARSE ERROR! In 'strtol()', errno ") + std::to_string(errno) + std::string(" while reading ") + std::string(1, buffer[pos]));
        }
        else if (end > str) {
            incPos(static_cast<intptr_t>(end - str));
            return static_cast<int>(number);
        }
    }

    void StreamBuffer::check_refill_buffer() {
        if (pos >= size - offset) {
            pos = 0;
            if (offset > 0) {
                std::copy(&buffer[size - offset], &buffer[size], buffer);
            }
            size = offset + gzread(in, buffer + offset, buffer_size - offset);
            offset = 0;
            if (size < buffer_size) {
                return;
            }
            // make sure buffer ends with newline (assert: does not cut a number)
            while (buffer[size - offset - 1] != '\n' && offset < size) {
                offset++;
            }
        }
    }

}
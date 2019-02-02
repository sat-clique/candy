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

#include <stdexcept>
#include <limits>
#include <algorithm>

#include "candy/utils/StreamBuffer.h"
#include "candy/frontend/Exceptions.h"

namespace Candy {

    void StreamBuffer::skipLine() {
        while (buf[pos] != '\n') {
            if (eof()) return;
            incPos(1);
        }
        incPos(1);
    }

    void StreamBuffer::skipWhitespace() {
        while ((buf[pos] >= 9 && buf[pos] <= 13) || buf[pos] == 32) {
            incPos(1);
        }
    }

    bool StreamBuffer::skipString(const char* str) {
        for (; *str != '\0'; ++str, incPos(1)) {
            if (*str != buf[pos]) {
                return false;
            }
        }
        return true;
    }

    int StreamBuffer::readInteger() {
        char* end = NULL;
        char* str = reinterpret_cast<char*>(&buf[pos]);
        errno = 0;
        long number = strtol(str, &end, 10);
        if (end > str) {
            if (errno == ERANGE || number <= std::numeric_limits<int>::min()/2 || number >= std::numeric_limits<int>::max()/2) {
                throw ParserException("PARSE ERROR! Variable " + std::to_string(number) + " is out of range");
            }

            if (errno != 0) {// && errno != 25 && errno != 29) {
                // After strtol, for reasons unknown, ERRNO=25 ('Not a typewriter') when acceptance tests are run from gTest
                // After strtol, for reasons unknown, ERRNO=29 ('Illegal seek') when reading from stdin
                char buffer[1024];
                std::sprintf(buffer, "PARSE ERROR! ERRNO=%i, in 'strtol' while reading '%.8s ..'\n", errno, buf.get()+pos);
                throw ParserException(std::string(buffer));
            }

            incPos(static_cast<intptr_t>(end - str));

            return static_cast<int>(number);
        }
        else if (errno != 0) {
            char buffer[1024];
            std::sprintf(buffer, "PARSE ERROR! ERRNO=%i, in 'strtol' while reading '%.8s ..'\n", errno, buf.get()+pos);
            throw ParserException(std::string(buffer));
        }
        else if (eof()) {
            throw ParserException("PARSE ERROR! Unexpected end of file");
        }
        else {
            char buffer[1024];
            std::sprintf(buffer, "PARSE ERROR! Expected integer but got unexpected char while attempting to read '%.8s ..'\n", buf.get()+pos);
            throw ParserException(std::string(buffer));
        }
    }

    void StreamBuffer::assureLookahead() {
        if (pos >= size - offset) {
            pos = 0;
            if (offset > 0) {
                std::copy(&buf[size - offset], &buf[size], buf.get());
            }
            size = offset + gzread(in, buf.get()+offset, buffer_size-offset);
            offset = 0;
            if (size < buffer_size) {
                return;
            }
            // make sure buffer ends with newline (assert: does not cut a number)
            while (buf[size - offset - 1] != '\n' && offset < size) {
                offset++;
            }
        }
    }

}
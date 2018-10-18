#include <stdexcept>
#include <limits>
#include <algorithm>

#include "candy/utils/StreamBuffer.h"

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
        long number = strtol(str, &end, 10);
        if (end > str) {
            if (errno == ERANGE || number <= std::numeric_limits<int>::min()/2 || number >= std::numeric_limits<int>::max()/2) {
                fprintf(stderr, "PARSE ERROR! Variable is out of integer-range\n");
                exit(3);
            }

            if (errno != 0 && errno != 25 && errno != 29) {
                // After strtol, for reasons unknown, ERRNO=25 ('Not a typewriter') when acceptance tests are run from gTest
                // After strtol, for reasons unknown, ERRNO=29 ('Illegal seek') when reading from stdin
                fprintf(stderr, "PARSE ERROR! ERRNO=%i, in 'strtol' while reading '%.8s ..'\n", errno, buf.get()+pos);
                throw std::runtime_error("");
            }

            incPos(static_cast<intptr_t>(end - str));

            return static_cast<int>(number);
        }
        else if (errno != 0) {
            fprintf(stderr, "PARSE ERROR! ERRNO=%i, in 'strtol' while attempting to read '%.8s ..'\n", errno, buf.get()+pos);
            throw std::runtime_error("");
        }
        else if (eof()) {
            fprintf(stderr, "PARSE ERROR! Unexpected end of file\n");
            exit(3);
        }
        else {
            fprintf(stderr, "PARSE ERROR! Expected integer but got unexpected char while attempting to read '%.8s ..'\n", buf.get()+pos);
            exit(3);
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
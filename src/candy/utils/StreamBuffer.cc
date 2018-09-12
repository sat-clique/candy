#include "candy/utils/StreamBuffer.h"
#include <stdexcept>

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
        if (end != str) {
            if (errno == ERANGE || number <= INT_MIN || number >= INT_MAX) {
                fprintf(stderr, "PARSE ERROR! Variable is out of integer-range\n"), exit(3);
            }
            if (errno != 0 && errno != 25) {
                // For reasons unknown ERRNO=25 when acceptance tests are run from gTest
                fprintf(stderr, "PARSE ERROR! ERRNO=%i, in 'strtol' while reading '%.8s ..'\n", errno, buf.get()+pos);
                throw std::runtime_error("");
            }

            incPos(reinterpret_cast<intptr_t>(end - str));

            return static_cast<int>(number);
        }
        else if (errno != 0) {
            fprintf(stderr, "PARSE ERROR! ERRNO=%i, in 'strtol' while attempting to read '%.8s ..'\n", errno, buf.get()+pos);
            throw std::runtime_error("");
        }
        else if (eof()) {
            fprintf(stderr, "PARSE ERROR! Unexpected end of file\n");
            throw std::runtime_error("");
        }
        else {
            fprintf(stderr, "PARSE ERROR! Expected integer but got unexpected char\n");
            throw std::runtime_error("");
            exit(3);
        }
    }

    void StreamBuffer::assureLookahead() {
        if (pos >= size - offset) {
            pos = 0;
            if (offset > 0) {
                // fprintf(stderr, "Before Copy: %.*s\n", size, buf.get());
                std::copy(&buf[size - offset], &buf[size], buf.get());
                // fprintf(stderr, "After Copy: %.*s\n", size, buf.get());
            }
            size = offset + gzread(in, buf.get()+offset, buffer_size-offset);
            // fprintf(stderr, "After Read: %.*s\n", size, buf.get());
            offset = 0;
            if (size < buffer_size) {
                return;
            }
            // make sure buffer ends with newline (assert: does not cut a number)
            while (buf[size - offset - 1] != '\n') {
                offset++;
            }
            fprintf(stderr, "After all: pos %i, offset %i, size %i\n", pos, offset, size);
        }
    }

}
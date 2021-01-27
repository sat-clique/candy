/*************************************************************************************************
GBDHash -- Copyright (c) 2020, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef StreamBuffer2_h
#define StreamBuffer2_h

#include <iostream>
#include <limits>
#include <archive.h>
#include <archive_entry.h>

class ParserException : public std::exception {
public:
    explicit ParserException(const std::string& what) noexcept : m_what(what) { }
    const char* what() const noexcept {
        return m_what.c_str();
    }
            
private:
    std::string m_what;
};

class StreamBuffer {
    struct archive* file;
    
    unsigned int buffer_size;
    char* buffer;
    
    unsigned int pos; // current read positition
    unsigned int end; // 1+last valid position
    bool end_of_file; // true when last chunk of file was read to buffer

    void check_refill_buffer() {
        if (pos >= end && !end_of_file) {
            pos = 0;
            if (end > 0 && end < buffer_size) {
                std::copy(buffer + end, buffer + buffer_size, buffer);
                end = buffer_size - end;
            } else {
                end = 0;
            }
            end += archive_read_data(file, buffer + end, buffer_size - end);
            if (end < buffer_size) {
                end_of_file = true;
            } else {
                while (!isspace(buffer[end-1])) {// align buffer with word-end
                    end--;
                }
            }
        }
    }

public:
    StreamBuffer(const char* filename) : buffer_size(16384), pos(0), end(0), end_of_file(false) {
        file = archive_read_new();
        archive_read_support_filter_all(file);
        archive_read_support_format_raw(file);
        int r = archive_read_open_filename(file, filename, buffer_size);
        if (r != ARCHIVE_OK) {
            throw ParserException(std::string("Error opening file."));
        }
        struct archive_entry *entry;
        r = archive_read_next_header(file, &entry);
        if (r != ARCHIVE_OK) {
            throw ParserException(std::string("Error reading header."));
        }
        buffer = new char[buffer_size];
        check_refill_buffer();
    }

    ~StreamBuffer() {
        archive_read_free(file);
        delete[] buffer;
    }

    /** Skip until the end of the next newline (+subsequent whitespace) */
    void skipLine() {
        while (!eof() && (!isspace(buffer[pos]) || isblank(buffer[pos]))) {
            incPos(1);
        }
        skipWhitespace();
    }

    void skipWhitespace() {
        while (!eof() && isspace(buffer[pos])) {
            incPos(1);
        }
    }

    /** Skip given sequence of character (+throw exceptions if input deviates) */
    void skipString(const char* str) {
        for (; *str != '\0'; ++str, incPos(1)) {
            if (*str != buffer[pos] || eof()) {
                throw ParserException(std::string("PARSE ERROR! Expected '") + std::string(str) + std::string("' but found ") + std::string(1, buffer[pos]));
            }
        }
    }

    int readInteger() {
        skipWhitespace();
        if (eof()) return 0; //throw ParserException(std::string("PARSE ERROR! Unexpected end of file"));

        char* str = buffer + pos;
        char* end = NULL;
                
        errno = 0;
        long number = strtol(str, &end, 10);

        if (errno == ERANGE || 2*labs(number) >= std::numeric_limits<uint32_t>::max()) {
            throw ParserException(std::string("PARSE ERROR! Variable out of supported range (32 bits): ") + std::to_string(number));
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
        else {
            throw ParserException(std::string("PARSE ERROR! Unexpected character ") + std::string(1, buffer[pos]));
        }
    }

    int operator *() const {
        return eof() ? EOF : buffer[pos];
    }

    void operator ++() {
        incPos(1);
    }

    void incPos(unsigned int inc) {
        pos += inc;
        check_refill_buffer();
    }

    bool eof() const {
        return (pos >= end) && end_of_file;
    }

};

#endif
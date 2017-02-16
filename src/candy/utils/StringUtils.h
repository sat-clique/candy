/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
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

#ifndef X_0C4C3434_BF63_4359_B6AC_6BFAC187A2CA_STRINGUTILS_H
#define X_0C4C3434_BF63_4359_B6AC_6BFAC187A2CA_STRINGUTILS_H

#include <string>
#include <sstream>
#include <vector>

namespace Candy {
    /**
     * Tokenizes \p input, using whitespace as a delimiter, and returns
     * a representation of the tokenized strings as a vector of elements
     * of type T (e.g. unsigned integer).
     */
    template<typename T>
    std::vector<T> tokenizeByWhitespace(const std::string &input) {
        std::vector<T> result;
        std::stringstream inpStream{input};
        
        T tmp;
        while(inpStream >> tmp) {
            result.push_back(tmp);
        }
        
        return result;
    }
}

#endif

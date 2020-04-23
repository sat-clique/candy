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

#ifndef X_295AB8F2_3BF4_4A76_B68B_D31FBF4C2A5B_H
#define X_295AB8F2_3BF4_4A76_B68B_D31FBF4C2A5B_H

#include <string>
#include <exception>

namespace Candy {
    /**
     * \ingroup CandyFrontend
     *
     * \brief An exception intended to be thrown when the given problem instance is
     *   not suitable for the algorithm intended to solve it.
     */
    class UnsuitableProblemException : public std::exception {
    public:
        explicit UnsuitableProblemException(const std::string& what) noexcept : m_what(what) { }
        const char* what() const noexcept {
            return m_what.c_str();
        }

    private:
        std::string m_what;
    };

    /**
     * \ingroup CandyFrontend
     *
     * \brief An exception intended to be thrown when the given file can not be parsed.
     */
    class ParserException : public std::exception {
    public:
        explicit ParserException(const std::string& what) noexcept : m_what(what) { }
        const char* what() const noexcept {
            return m_what.c_str();
        }
                
    private:
        std::string m_what;
    };
}

#endif

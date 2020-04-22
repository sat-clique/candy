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

#ifndef SRC_CANDY_CORE_CERTIFICATE_H_
#define SRC_CANDY_CORE_CERTIFICATE_H_

#include <vector>
#include <cstring>
#include <fstream>
#include <iostream>

#include "candy/core/SolverTypes.h"
#include "candy/core/clauses/Clause.h"

namespace Candy {

class Clause;

class Certificate {
private:
    bool active;
    std::ofstream out;

    template<typename Iterator>
    inline void printLiterals(Iterator it, Iterator end) {
        for(; it != end; it++) {
            out << (it->var() + 1) * (it->sign() ? -1 : 1) << " ";
        }
        out << "0" << std::endl;
    }

public:
    Certificate(const char* _out) : active(false), out() {
        out.open(_out, std::ios::out | std::ios::trunc );

        if (out.is_open()) {
            this->active = true;
        }
    }

    ~Certificate() {
        close();
    }

    inline void close() {
        if (out.is_open()) out.close();
        active = false;
    }

    inline void proof() {
        if (active) {
            out << "0" << std::endl;
        }
    }

    template<typename Iterator>
    inline void added(Iterator it, Iterator end) {
        if (active) {
            printLiterals(it, end);
        }
    }

    template<typename Iterator>
    inline void removed(Iterator it, Iterator end) {
        if (active) {
            out << "d ";
            printLiterals(it, end);
        }
    }
    
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CERTIFICATE_H_ */

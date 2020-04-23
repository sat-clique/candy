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

#include "candy/core/DRATChecker.h"

#include "candy/utils/StreamBuffer.h"
#include "candy/utils/Exceptions.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <fstream>
#include <iostream>

namespace Candy {

bool DRATChecker::check_proof(const char* filename) {
    gzFile in = gzopen(filename, "rb");
    if (in == NULL) {
        throw ParserException("ERROR! Could not open file");
    }
    bool result = check_proof(in);
    gzclose(in);
    return result;
}

long DRATChecker::proof_size(const char* filename) {
    struct stat stat_buf;
    int rc = stat(filename, &stat_buf);
    return (rc == 0) ? stat_buf.st_size : -1;
}

bool DRATChecker::check_proof(gzFile input_stream) {
    static Cl lits;
    StreamBuffer in(input_stream);
    in.skipWhitespace();
    while (!in.eof()) {
        if (*in == 'c') {
            in.skipLine();
        }
        else if (*in == 'd') {
            lits.clear();
            for (int plit = in.readInteger(); plit != 0; plit = in.readInteger()) {
                lits.push_back(Lit(abs(plit)-1, plit < 0));
            }
            check_clause_remove(lits.begin(), lits.end());
        }
        else {
            lits.clear();
            for (int plit = in.readInteger(); plit != 0; plit = in.readInteger()) {
                lits.push_back(Lit(abs(plit)-1, plit < 0));
            }
            check_clause_add(lits.begin(), lits.end());
        }
        in.skipWhitespace();
    }
}

template <typename Iterator>
bool DRATChecker::check_clause_add(Iterator begin, Iterator end) {
    return true;
}

template <typename Iterator>
bool DRATChecker::check_clause_remove(Iterator begin, Iterator end) {
    return true;
}


}

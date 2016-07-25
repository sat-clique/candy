/****************************************************************************************[Dimacs.h]
 Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 Copyright (c) 2007-2010, Niklas Sorensson

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

#ifndef Glucose_Dimacs_h
#define Glucose_Dimacs_h

#include <stdio.h>

#include "utils/ParseUtils.h"
#include "core/SolverTypes.h"

#include <vector>
using namespace std;

namespace Glucose {

class Dimacs {

  For problem;
  int nVars = 0;

  int headerVars = 0;
  int headerClauses = 0;

public:

  Dimacs(gzFile input_stream) {

    parse_DIMACS(input_stream);

    if (headerVars != nVars) {
      fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of variables.\n");
    }

    if (headerClauses != (int)problem.size()) {
      fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
    }
  }

  For& getProblem() {
    return problem;
  }

  int getNVars() {
    return nVars;
  }

private:

  void readClause(StreamBuffer& in, vector<Lit>* lits) {
    for (int plit = parseInt(in); plit != 0; plit = parseInt(in)) {
      int var = abs(plit);
      if (var > nVars) nVars = var;
      lits->push_back(mkLit(var-1, plit < 0));
    }
  }

  void parse_DIMACS(gzFile input_stream) {
    StreamBuffer in(input_stream);
    skipWhitespace(in);
    while (*in != EOF) {
      if (*in == 'p') {
        if (eagerMatch(in, "p cnf")) {
          headerVars = parseInt(in);
          headerClauses = parseInt(in);
          problem.reserve(headerClauses);
        }
        else {
          printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
        }
      }
      else if (*in == 'c' || *in == 'p') {
        skipLine(in);
      }
      else {
        vector<Lit>* lits = new vector<Lit>();
        readClause(in, lits);
        problem.push_back(lits);
      }
      skipWhitespace(in);
    }
  }

};

}

#endif

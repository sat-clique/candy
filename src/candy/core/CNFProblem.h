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

#include "utils/ParseUtils.h"
#include "core/SolverTypes.h"

#include <vector>

namespace Candy {

class CNFProblem {

  For problem;
  int maxVars = 0;

  int headerVars = 0;
  int headerClauses = 0;

public:

  CNFProblem() { }

  For& getProblem() {
    return problem;
  }
    
  const For& getProblem() const {
    return problem;
  }

  int nVars() const {
    return maxVars;
  }

  int nClauses() {
    return (int)problem.size();
  }

  int newVar() {
    maxVars++;
    return maxVars-1;
  }

  bool readDimacsFromStdout() {
    gzFile in = gzdopen(0, "rb");
    if (in == NULL) {
      printf("ERROR! Could not open file: <stdin>");
      return false;
    }
    parse_DIMACS(in);
    gzclose(in);
    return true;
  }

  bool readDimacsFromFile(const char* filename) {
    gzFile in = gzopen(filename, "rb");
    if (in == NULL) {
      printf("ERROR! Could not open file: %s\n", filename);
      return false;
    }
    parse_DIMACS(in);
    gzclose(in);
    return true;
  }

  void readClause(Lit plit) {
    Cl* lits = new Cl();
    lits->push_back(plit);
    maxVars = std::max(maxVars, var(plit)+1);
    problem.push_back(lits);
  }

  void readClause(Lit plit1, Lit plit2) {
    Cl* lits = new Cl();
    lits->push_back(plit1);
    lits->push_back(plit2);
    maxVars = std::max(maxVars, std::max(var(plit1),var(plit2))+1);
    problem.push_back(lits);
  }

  void readClause(std::initializer_list<Lit> list) {
    Cl* lits = new Cl(list);
    maxVars = var(std::max(list))+1;
    problem.push_back(lits);
  }

  void readClause(Cl& in) {
    Cl* lits = new Cl(in);
    maxVars = var(*std::max_element(in.begin(), in.end()))+1;
    problem.push_back(lits);
  }

  void readClauses(For& f) {
    for (Cl* c : f) {
      readClause(*c);
    }
  }


private:

  void readClause(Glucose::StreamBuffer& in) {
    Cl* lits = new Cl();
    for (int plit = parseInt(in); plit != 0; plit = parseInt(in)) {
      int var = abs(plit);
      if (var > maxVars) maxVars = var;
      lits->push_back(Glucose::mkLit(var-1, plit < 0));
    }
    problem.push_back(lits);
  }

  void parse_DIMACS(gzFile input_stream) {
    Glucose::StreamBuffer in(input_stream);
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
        readClause(in);
      }
      skipWhitespace(in);
    }
    if (headerVars != maxVars) {
      fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of variables.\n");
    }
    if (headerClauses != (int)problem.size()) {
      fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
    }
  }

};

}

#endif

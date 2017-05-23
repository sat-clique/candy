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

#ifndef CNFProblem_h
#define CNFProblem_h

#include "candy/utils/ParseUtils.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/Certificate.h"

#include <vector>

namespace Candy {

class CNFProblem {

  For problem;

  Certificate* certificate;

  int maxVars = 0;

  int headerVars = 0;
  int headerClauses = 0;

public:
  CNFProblem() {
      certificate = new Certificate(nullptr, false);
  }

  CNFProblem(Certificate* _certificate) : certificate(_certificate) {
  }

  For& getProblem();
  const For& getProblem() const;

  inline int nVars() const {
      return maxVars;
  }

  inline int nClauses() const {
      return (int)problem.size();
  }

  inline int newVar() {
      maxVars++;
      return maxVars-1;
  }

  bool readDimacsFromStdout();
  bool readDimacsFromFile(const char* filename);

  inline void readClause(Lit plit) {
      readClause({plit});
  }

  inline void readClause(Lit plit1, Lit plit2) {
      readClause({plit1, plit2});
  }

  inline void readClause(std::initializer_list<Lit> list) {
      readClause(list.begin(), list.end());
  }

  inline void readClause(Cl cl) {
      readClause(cl.begin(), cl.end());
  }

  inline void readClauses(For& f) {
      for (Cl* c : f) {
          readClause(c->begin(), c->end());
      }
  }

  template <typename Iterator>
  inline void readClause(Iterator begin, Iterator end) {
      Cl* clause = new Cl(begin, end);
      std::sort(clause->begin(), clause->end());
      Lit prev = lit_Undef;
      auto insertion_point = clause->begin();
      for (Lit lit : *clause) {
          if (lit == ~prev) {
              delete clause;
              certificate->removed(begin, end);
              return; // reject tautological clauses
          }
          else if (lit != prev) {
              maxVars = std::max(maxVars, var(lit)+1);
              prev = lit;
              *insertion_point = lit;
              insertion_point++;
          }
          else {
              // skip redundant literals
          }
      }
      if (insertion_point != clause->end()) {
          certificate->added(clause->begin(), insertion_point);
          certificate->removed(begin, end);
          clause->erase(insertion_point, clause->end());
      }
      problem.push_back(clause);
  }


private:

  void readClause(Glucose::StreamBuffer& in);
  void parse_DIMACS(gzFile input_stream);

};

}

#endif

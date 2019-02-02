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

#ifndef NORMALIZEDCLAUSELIST_H_
#define NORMALIZEDCLAUSELIST_H_

#include <map>
#include <stdio.h>

#include "clauses/types/ClauseList.h"

namespace Dark {

class Literals;
class Cube;

/**
 * Only keep the satisfied literals (wrt. the given model) of each added clause
 */
class NormalizedClauseList: public Dark::ClauseList {
public:
  NormalizedClauseList(Cube* model);
  virtual ~NormalizedClauseList();

  void add(Literals* clause);
  void addAll(ClauseList* list);

  Literal normalize(Literal lit);
  Literal denormalize(Literal lit);

  int getNumberOfVariables() {
    return nVars;
  }

private:
  Cube* model;
  int nVars;

  map<Literal, Literal>* denormalizedLiteral;
};

} /* namespace Analyzer */
#endif /* NORMALIZEDCLAUSELIST_H_ */

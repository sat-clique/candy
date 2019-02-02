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

#include "NormalizedClauseList.h"

#include "clauses/types/ClauseList.h"
#include "clauses/types/Cube.h"

namespace Dark {

NormalizedClauseList::NormalizedClauseList(Cube* model) {
  this->model = model;
  this->denormalizedLiteral = new map<Literal, Literal>();

  this->nVars = 0;
  for (std::vector<Literal>::iterator it = model->begin(); it != model->end(); ++it) {
    Literal lit = *it;
    if (var(lit)+1 > this->nVars) this->nVars = var(lit)+1;
  }
}

NormalizedClauseList::~NormalizedClauseList() {
  freeClauses();
  delete denormalizedLiteral;
}

void NormalizedClauseList::add(Literals* clause) {
  Literals* normalizedClause = new Literals();
  for (std::vector<Literal>::iterator it = clause->begin(); it != clause->end(); ++it) {
    Literal lit = *it;
    if (model->contains(lit)) {
      normalizedClause->add(normalize(lit));
    }
  }

  ClauseList::add(normalizedClause);
}

void NormalizedClauseList::addAll(ClauseList* list) {
  for(int i = 0; i < list->size(); i++) {
    this->add(list->get(i));
  }
}

Literal NormalizedClauseList::normalize(Literal lit) {
  if (sign(lit)) {
    (*denormalizedLiteral)[lit] = ~lit;
    (*denormalizedLiteral)[~lit] = lit;
    return ~lit;
  } else {
    (*denormalizedLiteral)[lit] = lit;
    (*denormalizedLiteral)[~lit] = ~lit;
    return lit;
  }
}

Literal NormalizedClauseList::denormalize(Literal lit) {
  return (*denormalizedLiteral)[lit];
}

} /* namespace Analyzer */

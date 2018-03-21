/*
 * NormalizedClauseList.cpp
 *
 *  Created on: 10.01.2014
 *      Author: markus
 */

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

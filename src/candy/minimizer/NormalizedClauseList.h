/*
 * PurifiedClauseList.h
 *
 *  Created on: 10.01.2014
 *      Author: markus
 */

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

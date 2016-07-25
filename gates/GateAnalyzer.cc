/*
 * GateAnalyzer.cc
 *
 *  Created on: Jul 21, 2016
 *      Author: markus
 */

#include "gates/GateAnalyzer.h"

using namespace std;
using namespace Glucose;

GateAnalyzer::GateAnalyzer(Dimacs& dimacs, int maxTries) {
  this->formula = dimacs.getProblem();
  this->nVars = dimacs.getNVars();
  this->maxTries = maxTries;
  this->gates = new For(nVars);
  this->inputs.resize(2 * nVars, false);
  this->index.resize(2 * nVars);
}

// heuristically select clauses
vector<Cl*>& GateAnalyzer::selectClauses() {
  unsigned int min = INT_MAX;
  int minLit = -1;
  for (int l = 0; l < 2*nVars; l++) {
    if (index[l].size() > 0 && index[l].size() < min) {
      min = index[l].size();
      minLit = l;
    }
  }
  if (minLit == -1) vector<Cl*>();
  return index[minLit];
}

void GateAnalyzer::analyze() {
  // populate index (except for unit-clauses, they go to roots immediately)
  for (Cl* c : formula)
    if (c->size() == 1) roots.push_back(c);
    else for (Lit l : *c) index[l].push_back(c);

  // start with unit clauses
  set<Lit> next;
  for (Cl* c : roots) for (Lit l : *c) next.insert(l);
  analyze(next);

  // clause selection loop
  for (int k = 0; k < maxTries; k++) {
    next.clear();
    vector<Cl*>& clauses = selectClauses();
    for (Cl* c : clauses) {
      next.insert(c->begin(), c->end());
    }
    removeFromIndex(clauses);
    analyze(next);
  }
}

// main analysis routine
void GateAnalyzer::analyze(set<Lit>& roots) {
  vector<Lit> literals(roots.begin(), roots.end());

  for (Lit l : roots) inputs[l]++;

  while (literals.size()) {
    Lit o = literals.back();
    literals.pop_back();

    For& f = index[~o], g = index[o];
    if (f.size() > 0 && isBlocked(o, f, g)) {
      bool mono = !inputs[o] || !inputs[~o];
      set<Lit> s, t;
      for (Cl* c : f) for (Lit l : *c) if (l != ~o) s.insert(l);
      if (!mono)  for (Cl* c : g) for (Lit l : *c) if (l != o) t.insert(~l);
      bool gate = mono || (s == t
                  && (f.size() == 1 && fixedClauseSize(g, 2)
                  || g.size() == 1 && fixedClauseSize(f, 2)
                  || s.size() == 4 && s.find(~(*s.begin())) != s.end() && fixedClauseSize(f, 3)) );
      if (gate) {
        nGates++;
        (*gates)[var(o)] = new Cl(s.begin(), s.end());
        literals.insert(literals.end(), s.begin(), s.end());
        for (Lit l : s) {
          inputs[l]++;
          if (!mono) inputs[~l]++;
        }
        removeFromIndex(f);
        removeFromIndex(g);
      }
    }
  }
}

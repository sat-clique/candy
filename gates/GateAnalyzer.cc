/*
 * GateAnalyzer.cc
 *
 *  Created on: Jul 21, 2016
 *      Author: markus
 */

#include "gates/GateAnalyzer.h"

using namespace std;
using namespace Glucose;

GateAnalyzer::GateAnalyzer(Dimacs& dimacs, int maxTries) :
	formula (dimacs.getProblem()),
	nVars (dimacs.getNVars()),
	maxTries (maxTries) {
  gates = new For(nVars);
  inputs.resize(2 * nVars, false);
  index.resize(2 * nVars);
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

// clause patterns of full encoding
bool GateAnalyzer::completePattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs) {
  // precondition: fwd and bwd constrain exactly the same inputs (in opposite polarity) and fwd blocks bwd on the output literal
  set<Var> vars;
  for (Lit l : inputs) vars.insert(var(l));
  return fwd.size() == bwd.size() && 2*fwd.size() == pow(2, vars.size()) && 2*vars.size() == inputs.size();
}

// clause patterns of full encoding
bool GateAnalyzer::fullPattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs) {
  // precondition: fwd and bwd constrain exactly the same inputs (in opposite polarity) and fwd blocks bwd on the output literal
  set<Var> vars;
  for (Lit l : inputs) vars.insert(var(l));
  bool fullOr = fwd.size() == 1 && fixedClauseSize(bwd, 2);
  bool fullAnd = bwd.size() == 1 && fixedClauseSize(fwd, 2);
//  bool fullBXor = inputs.size() == 4 && vars.size() == 2 && fixedClauseSize(fwd, 3);
  return fullOr || fullAnd || completePattern(fwd, bwd, inputs);
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
      if (!mono) for (Cl* c : g) for (Lit l : *c) if (l != o) t.insert(~l);
      bool gate = mono || (s == t && fullPattern(f, g, s));
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

/*
 * GateAnalyzer.cc
 *
 *  Created on: Jul 21, 2016
 *      Author: markus
 */

#include "gates/GateAnalyzer.h"

using namespace std;

GateAnalyzer::GateAnalyzer(int maxTries) {
  this->maxTries = maxTries;
}

int GateAnalyzer::getNGates() {
  return nGates;
}

Formula* GateAnalyzer::analyze(Formula& formula, int nVars) {
  Formula* gates = new Formula(nVars);
  vector<char> inp(2 * nVars, false);
  vector<Formula> index(2 * nVars);

  Cl roots;

  for (Cl* c : formula)
    if (c->size() == 1)
      roots.push_back((*c)[0]);
    else
      for (Lit l : *c)
        index[l].push_back(c);

  int k = 0;

  while (k < maxTries || roots.size()) {
    if (!roots.size()) { // select clause
      k++;
      unsigned int min = INT_MAX;
      int minLit = -1;
      for (int l = 0; l < 2*nVars; l++) {
        if (index[l].size() > 0 && index[l].size() < min) {
          min = index[l].size();
          minLit = l;
        }
      }
      if (minLit == -1) break;
      set<Lit> next;
      for (Cl* c : index[minLit]) {
        next.insert(begin(*c), end(*c));
      }
      index[minLit].clear();
      for (Lit l : next) inp[l]++;
      roots.insert(roots.end(), begin(next), end(next));
    }

    Lit o = roots.back();
    roots.pop_back();
    Formula& f = index[~o], g = index[o];
    if (f.size() > 0 && isBlocked(o, f, g)) {
      bool mono = !inp[o] || !inp[~o];
      set<Lit> s, t;
      for (Cl* c : f) for (Lit l : *c) if (l != ~o) s.insert(l);
      if (!mono)  for (Cl* c : g) for (Lit l : *c) if (l != o) t.insert(~l);
      bool gate = mono || (s == t
                  && (f.size() == 1 && fixedClauseSize(g, 2)
                  || g.size() == 1 && fixedClauseSize(f, 2)
                  || s.size() == 4 && s.find(~(*s.begin())) != s.end() && fixedClauseSize(f, 3)) );
      if (gate) {
        nGates++;
        (*gates)[var(o)] = new Cl(begin(s), end(s));
        roots.insert(roots.end(), begin(s), end(s));
        for (Lit l : s) {
          inp[l] = true;
          if (!mono) inp[~l] = true;
        }
        for (Formula e : { f, g })
          for (Cl* c : e)
            for (Lit l : *c) {
              Formula& h = index[l];
              h.erase(remove(begin(h), end(h), c), end(h));
            }
      }
    }
  }

  return gates;
}

bool GateAnalyzer::isBlocked(Lit o, Cl& a, Cl& b) {
  for (Lit c : a)
    for (Lit d : b)
      if (c != ~o && c == ~d)
        return true;
  return false;
}

bool GateAnalyzer::isBlocked(Lit o, Formula& f, Formula& g) {
  for (Cl* a : f)
    for (Cl* b : g)
      if (!isBlocked(o, *a, *b))
        return false;
  return true;
}

bool GateAnalyzer::fixedClauseSize(Formula& f, unsigned int n) {
  for (Cl* c : f)
    if (c->size() != n)
      return false;
  return true;
}

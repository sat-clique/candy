#ifndef MGA
#define MGA

#include <cstdlib>
#include <algorithm>

#include <vector>
#include <set>

#include <climits>

#include "core/Dimacs.h"
using namespace Glucose;

using namespace std;

#include "core/SolverTypes.h"

using namespace Candy;

class GateAnalyzer {

public:
  GateAnalyzer(Dimacs& dimacs, int maxTries = 0);

  // main analysis routines:
  void analyze();
  void analyze(set<Lit>& roots);

  // public getters:
  int getNGates() { return nGates; }

private:
  // problem to analyze:
  For& formula;
  int nVars;

  // control structures:
  vector<Cl*> roots;
  vector<For> index; // occurrence lists
  vector<char> inputs; // flags to check if both polarities of literal are used as input (monotonicity)

  // heuristic configuration:
  int maxTries = 1;

  // statistics:
  int nGates = 0;
  For* gates;

  // clause selection heuristic
  vector<Cl*>& selectClauses();

  // some helpers:
  bool isBlocked(Lit o, Cl& a, Cl& b) {
    for (Lit c : a) for (Lit d : b) if (c != ~o && c == ~d) return true;
    return false;
  }

  bool isBlocked(Lit o, For& f, For& g) {
    for (Cl* a : f) for (Cl* b : g) if (!isBlocked(o, *a, *b)) return false;
    return true;
  }

  bool fixedClauseSize(For& f, unsigned int n) {
    for (Cl* c : f) if (c->size() != n) return false;
    return true;
  }

  void removeFromIndex(vector<Cl*> clauses) {
    for (Cl* c : clauses) for (Lit l : *c) {
      For& h = index[l];
      h.erase(remove(h.begin(), h.end(), c), h.end());
    }
  }

};

#endif

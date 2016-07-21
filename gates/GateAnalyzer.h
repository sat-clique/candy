#ifndef MGA
#define MGA

#include <cstdlib>
#include <algorithm>

#include <vector>
#include <set>

#include <climits>

using namespace std;

#include "core/SolverTypes.h"

typedef Glucose::Lit Lit;
typedef vector<Lit> Cl;
typedef vector<Cl*> Formula;

class GateAnalyzer {

public:
  GateAnalyzer(int maxTries = 0);

  int getNGates();
  Formula* analyze(Formula& formula, int nVars);

private:
  int maxTries = 1;

  int nGates = 0;

  bool isBlocked(Lit o, Cl& a, Cl& b);
  bool isBlocked(Lit o, Formula& f, Formula& g);
  bool fixedClauseSize(Formula& f, unsigned int n);

};

#endif

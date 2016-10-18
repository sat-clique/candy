#ifndef MGA
#define MGA

#include <cstdlib>
#include <algorithm>

#include <vector>
#include <set>

#include <climits>

#include "../core/CNFProblem.h"
using namespace Glucose;

using namespace std;

#include "core/SolverTypes.h"
#include "core/Utilities.h"
#include "core/Solver.h"

class GateAnalyzer {

public:
  GateAnalyzer(CNFProblem& dimacs);

  // main analysis routines:
  void analyze();
  void analyze(set<Lit>& roots);

  // public getters:
  int getNGates() { return nGates; }
  For* getGates() { return gates; }

private:
  // problem to analyze:
  CNFProblem problem;
  Solver solver;

  // control structures:
  vector<Cl*> roots;
  vector<For> index; // occurrence lists
  vector<char> inputs; // flags to check if both polarities of literal are used as input (monotonicity)

  // heuristic configuration:
  int maxTries = 0;
  bool usePatterns = false;
  bool useSemantic = false;
  bool useHolistic = false;
  bool useLookahead = false;

  // statistics:
  int nGates = 0;
  For* gates;

  // debugging
  bool verbose = false;
  int debug_counter = 0;

  // clause selection heuristic
  Lit getRarestLiteral(vector<For>& index);

  // clause patterns of full encoding
  bool fullPattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs);
  bool completePattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs);
  bool semanticCheck(vector<Cl*>& fwd, vector<Cl*>& bwd, Var o);
  bool isFullGate(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs, Lit output);

  // some helpers:
  bool isBlocked(Lit o, Cl& a, Cl& b) {
    // assert ~o \in a and o \in b
    for (Lit c : a) for (Lit d : b) if (c != ~o && c == ~d) return true;
    return false;
  }

  bool isBlocked(Lit o, For& f, For& g) {
    // assert ~o \in f[i] and o \in g[j]
    for (Cl* a : f) for (Cl* b : g) if (!isBlocked(o, *a, *b)) return false;
    return true;
  }

  bool isBlocked(Lit o, Cl* c, For& f) {
    // assert ~o \in c and o \in f[i]
    for (Cl* a : f) if (!isBlocked(o, *c, *a)) return false;
    return true;
  }

  void saturate(For& source, For& target, For& blocked, Lit o) {
    // assert ~o \in source[i] and o \in blocked
    for (unsigned int i = 0; i < source.size(); i++) {
      int n = source.size();
      if (isBlocked(o, source[i], blocked)) {
        target.push_back(source[i]);
        std::swap(source[i], source[n-1]);
        source.pop_back();
      }
    }
  }

  bool checkIndexConsistency() {
    for (int i = 0; i < 2 * problem.nVars(); i++) {
      for (Cl* clause : index[i]) {
        if (find(clause->begin(), clause->end(), toLit(i)) == clause->end()) {
//          printf("Detected inconsistency for literal %s%i", sign(lit)?"-":"", var(lit)+1);
          return false;
        }
      }
    }
    return true;
  }

  void countIndex() {
    for (int i = 0; i < 2 * problem.nVars(); i++) {
      Lit lit = toLit(i);
      printf("(%s%i, %zu) ", sign(lit)?"-":"", var(lit)+1, index[i].size());
    }
  }

  bool isBlockedAfterVE(Lit o, For& f, For& g);

  bool fixedClauseSize(For& f, unsigned int n) {
    for (Cl* c : f) if (c->size() != n) return false;
    return true;
  }

  void removeFromIndex(vector<For>& index, Cl* clause) {
    for (Lit l : *clause) {
      For& h = index[l];
      h.erase(remove(h.begin(), h.end(), clause), h.end());
    }
  }

  void assertNotInIndex(vector<For>& index, Cl* clause) {
    for (Lit l : *clause) {
      assert(find(index[l].begin(), index[l].end(), clause) == index[l].end());
    }
  }

  void removeFromIndex(vector<For>& index, vector<Cl*>& clauses) {
    for (Cl* c : clauses) {
      removeFromIndex(index, c);
      assertNotInIndex(index, c);
    }
  }
};

#endif

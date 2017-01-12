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

typedef struct Gate {
  Lit out = lit_Undef;
  For fwd, bwd;
  bool notMono = false;
  vector<Lit> inp;

  inline bool isDefined() { return out != lit_Undef; }

  // Compatibility functions
  inline Lit getOutput() { return out; }
  inline For* getForwardClauses() { return &fwd; }
  inline For* getBackwardClauses() { return &bwd; }
  inline vector<Lit>* getInputs() { return &inp; }
  inline bool hasNonMonotonousParent() { return notMono; }
  // End of compatibility functions
} Gate;

class GateAnalyzer {

public:
  GateAnalyzer(CNFProblem& dimacs);

  // main analysis routines:
  void analyze();
  void analyze(set<Lit>& roots);

  // public getters:
  int getNGates() { return nGates; }
  vector<Cl*>* getGates() { return gates; }
  Gate& getGate(Lit output) { return (*gatesComplete)[output]; }

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
  bool useDecomposition = false;
  int decompMaxBlocks = 2;

  // analyzer output:
  int nGates = 0;
  vector<Cl*>* gates; // stores inputs for every output
  vector<Gate>* gatesComplete; // stores gate-struct for every output

  // debugging
  bool verbose = false;

  // clause selection heuristic
  Lit getRarestLiteral(vector<For>& index);

  // clause patterns of full encoding
  bool fullPattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs);
  bool completePattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs);
  bool semanticCheck(vector<Cl*>& fwd, vector<Cl*>& bwd, Var o);
  bool isFullGate(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs, Lit output);

  // work in progress:
  bool isBlockedAfterVE(Lit o, For f, For g);

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

  vector<For> buildIndexFromClauses(For& f) {
    vector<For> index(2 * problem.nVars());
    for (Cl* c : f) for (Lit l : *c) {
      index[l].push_back(c);
    }
    return index;
  }

};

#endif

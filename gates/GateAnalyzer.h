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


  void enumerateBlockedPairs() {
    for (Cl* c : problem.getProblem())
      if (c->size() == 1) roots.push_back(c);
      else for (Lit l : *c) index[l].push_back(c);
    int n = problem.nClauses() * 100;
    bool* blocks = (bool*)calloc(n, sizeof(bool));
    for (int v = 0; v < problem.nVars(); v++) {
      Lit l = mkLit(v, false);
      for (Cl* c1 : index[l]) {
        for (Cl* c2 : index[~l]) {
          int addr = ((long)c1 + (long)c2) % n;
          blocks[addr] = blocks[addr] || isBlocked(l, *c1, *c2);
        }
      }
    }
    int c = 0;
    for (int i = 0; i < n; i++) {
      if (blocks[i]) c++;
    }
    printf("%i of %i bits are set (%i * %i pairs)", c, n, problem.nClauses(), problem.nClauses());
  }

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

  // statistics:
  int nGates = 0;
  For* gates;

  // debugging
  bool verbose = false;

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

  bool isBlockedGreedyDecompose(Lit o, For f, For g) {
    vector<For> left, right;

    if (verbose) {
      printf("Decomposition with Output-literal %s%i\n", sign(o)?"-":"", var(o)+1);
    }
    // order the clauses by literal rarity
    vector<For> index = buildIndexFromClauses(f);

    while (!f.empty() && !g.empty() && left.size() <= (size_t)decompMaxBlocks) {
      left.push_back(For());
      right.push_back(For());

      Lit lit = getRarestLiteral(index);
      For next = index[lit];

      assert(next.size() > 0);

      removeFromIndex(index, next);

      assert(index[lit].empty());

      size_t size = f.size();
      for (Cl* c : next) {
    	  f.erase(std::remove(f.begin(), f.end(), c), f.end());
          assert(find(f.begin(), f.end(), c) == f.end());
      }
      assert(size > f.size());

      left.back().insert(left.back().end(), next.begin(), next.end());
      saturate(g, right.back(), left.back(), ~o);

      int n = left.back().size();
      saturate(f, left.back(), right.back(), o);
      for (size_t i = n; i < left.back().size(); i++) {
        removeFromIndex(index, left.back()[i]);
      }
    }

    if (verbose && g.empty()) {
      printf("Decomposition into:");
      for (size_t i = 0; i < left.size(); i++) {
        printf("Block %zu\n", i);
        printClauses(left[i]);
        printf("----------\n");
        printClauses(right[i]);
        printf("----------\n----------\n");
        assert(isBlocked(o, left[i], right[i]));
      }
    }

    return g.empty();
  }

  bool isBlockedAfterVE(Lit o, For f, For g) {
    For combs;
    // assert ~o \in f[i] and o \in g[j]
    for (Cl* a : f) for (Cl* b : g) {
      Cl* comb = new Cl();
      if (!isBlocked(o, *a, *b)) {
        comb->insert(comb->end(), a->begin(), a->end());
        comb->insert(comb->end(), b->begin(), b->end());
        comb->erase(std::remove_if(comb->begin(), comb->end(), [o](Lit l) { return var(l) == var(o); }), comb->end());
        combs.push_back(comb);
      }
    }

    set<Lit> uni;
    for (Cl* comb : combs) uni.insert(comb->begin(), comb->end());

    vector<Var> candidates;
    for (Lit l : *(combs[0])) candidates.push_back(var(l));
    vector<Var> next_candidates;
    for (int i = 1; i < combs.size() && !candidates.empty(); ++i) {
      vector<Var> next_combo;
      for (Lit l : *(combs[i])) next_combo.push_back(var(l));
      std::set_intersection(candidates.begin(), candidates.end(),
          next_combo.begin(), next_combo.end(), std::back_inserter(next_candidates));
      std::swap(candidates, next_candidates);
      next_candidates.clear();
    }

    if (candidates.empty()) return false;

    for (Var v : candidates) {
      for (For& comb : combs) {

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

  void removeFromIndex(vector<For>& index, vector<Cl*> clauses) {
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

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


  // precondition: ~o \in f[i] and o \in g[j]
  bool isBlockedAfterVE(Lit o, For f, For g) {
    // generate set of non-tautological resolvents
    For resolvents;
    for (Cl* a : f) for (Cl* b : g) {
      Cl* res = new Cl();
      if (!isBlocked(o, *a, *b)) {
        res->insert(res->end(), a->begin(), a->end());
        res->insert(res->end(), b->begin(), b->end());
        res->erase(std::remove_if(res->begin(), res->end(), [o](Lit l) { return var(l) == var(o); }), res->end());
        resolvents.push_back(res);
      }
    }
    if (resolvents.empty()) return true; // the set is trivially blocked

    // generate set of literals whose variable occurs in every non-taut. resolvent (by successive intersection of resolvents)
    set<Lit> candidates(resolvents[0]->begin(), resolvents[0]->end());
    for (Cl* resolvent : resolvents) {
      if (candidates.empty()) break;
      set<Lit> next_candidates;
      for (Lit lit1 : *resolvent) {
        for (Lit lit2 : candidates) {
          if (var(lit1) == var(lit2)) {
            next_candidates.insert(lit1);
            next_candidates.insert(lit2);
            break;
          }
        }
      }
      std::swap(candidates, next_candidates);
      next_candidates.clear();
    }
    if (candidates.empty()) return false; // no candidate output

    // generate set of input variables of candidate gate (o, f, g)
    set<Var> inputs;
    for (Cl* c : f) for (Lit l : *c) if (var(l) != var(o)) inputs.insert(var(l));
    for (Cl* c : g) for (Lit l : *c) if (var(l) != var(o)) inputs.insert(var(l));

    for (Lit candLit : candidates) {
      // generate candidate definition for output
      For fwd, bwd;

      for (Lit lit : { candLit, ~candLit })
      for (Cl* c : index[lit]) {
        // clauses of candidate gate (o, f, g) are still part of index (skip them)
        if (find(f.begin(), f.end(), c) == f.end()) continue;
        if (find(g.begin(), g.end(), c) == g.end()) continue;
        // use clauses that constrain the inputs of our candidate gate only
        bool is_subset = true;
        for (Lit l : *c) {
          if (find(inputs.begin(), inputs.end(), var(l)) == inputs.end()) {
            is_subset = false;
            break;
          }
        }
		if (is_subset) {
		  if (lit == ~candLit) fwd.push_back(c);
		  else bwd.push_back(c);
		}
      }

      // if candidate definition is functional
      // (check blocked state, in non-monotonic case also right-uniqueness <- use semantic holistic approach)
      bool monotonic = false;
      bool functional = false;
      if (isBlocked(candLit, fwd, bwd)) {
        // output is used monotonic, iff
        // 1. it is pure in the already decoded partial gate-structure (use input array)
        bool pure1 = !this->inputs[candLit];
        // 2. it is pure in f of the candidate gate-definition (o, f, g)
        bool pure2 = false;
        // 3. it is pure in the remaining formula (look at literals in index, bwd entails entire index)
        bool pure3 = false;
        monotonic =  pure1 && pure2 && pure3;
        if (!monotonic) {
          functional = semanticCheck(fwd, bwd, var(candLit));
        }

        // then local resolution is enough and then check resolve and check for gate-property again
        if (monotonic || functional) {
          // split resolvents
          For res_fwd, res_bwd;
          for (Cl* res : resolvents) {
            if (find(res->begin(), res->end(), ~candLit) == res->end()) {
              res_fwd.push_back(res);
            } else {
              res_bwd.push_back(res);
            }
          }
          if (isBlocked(~candLit, res_fwd, bwd) && isBlocked(~candLit, fwd, res_bwd)) return true;
        }
        else; // next candidate
      }
    }

    return false;
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

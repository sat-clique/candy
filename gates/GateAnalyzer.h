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

    // no non-tautological resolvents
    if (resolvents.empty()) return true;

    // generate set of input variables of candidate gate G = f and g
    set<Var> inputs;
    for (Cl* c : f) for (Lit l : *c) if (var(l) != var(o)) inputs.insert(var(l));
    for (Cl* c : g) for (Lit l : *c) if (var(l) != var(o)) inputs.insert(var(l));

    // generate candidate outputs (use intersection of all inputs in all non-tautological resolvents)
    // TODO: make sets from vectors
    vector<Var> candidate_vars;
    for (Lit l : *(resolvents[0])) candidate_vars.push_back(var(l));
    vector<Var> next_candidates;
    for (size_t i = 1; i < resolvents.size() && !candidate_vars.empty(); ++i) {
      vector<Var> vars;
      for (Lit l : *(resolvents[i])) vars.push_back(var(l));
      std::set_intersection(candidate_vars.begin(), candidate_vars.end(),
          vars.begin(), vars.end(), std::back_inserter(next_candidates));
      std::swap(candidate_vars, next_candidates);
      next_candidates.clear();
    }

    // no candidate output
    if (candidate_vars.empty()) return false;

    set<Lit> candidate_lits;
    for (Cl* resolvent : resolvents) {
      for (Lit l : *resolvent) {
        if (find(candidate_vars.begin(), candidate_vars.end(), var(l)) == candidate_vars.end()) {
          candidate_lits.insert(l);
        }
      }
    }

    for (Var cand : candidate_vars) {
      Lit output = mkLit(cand, false);

      bool pos_is_candidate = find(candidate_lits.begin(), candidate_lits.end(), output) != candidate_lits.end();
      bool neg_is_candidate = find(candidate_lits.begin(), candidate_lits.end(), ~output) != candidate_lits.end();
      // make sure fwd-clauses are usable in monotonic case
      if (!pos_is_candidate) output = ~output;

      // generate candidate definition for output
      For fwd, bwd;

      for (Cl* c : index[~output]) {
        // clauses of candidate gate are still part of index (filter them out)
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
		if (is_subset) fwd.push_back(c);
      }

      for (Cl* c : index[output]) {
        // clauses of candidate gate are still part of index (filter them out)
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
        if (is_subset) bwd.push_back(c);
      }

      if (pos_is_candidate && fwd.empty()) continue;
      if (neg_is_candidate && bwd.empty()) continue;

      // if candidate definition is functional
      // (check blocked state, in non-monotonic case also right-uniqueness <- use semantic holistic approach)
      bool functional = false;
      if (isBlocked(output, fwd, bwd)) {
        // output is used monotonic, iff
        // 1. it is pure in the candidate gate-definition
        // 2. it is pure in the current partial gate-structure
        // 3. it is pure in the remaining formula
        bool monotonic = !neg_is_candidate && !inputs[output] && bwd.size() == index[output].size();
        if (monotonic) {
          functional = true;
        } else {
          functional = semanticCheck(fwd, bwd, var(output));
        }
      }

      // then local resolution is enough and then check resolve and check for gate-property again
      if (functional) {
        // split resolvents
        For res_fwd, res_bwd;
        for (Cl* res : resolvents) {
          if (find(res->begin(), res->end(), ~output) == res->end()) {
            res_fwd.push_back(res);
          } else {
            res_bwd.push_back(res);
          }
        }
        if (isBlocked(~output, res_fwd, bwd) && isBlocked(~output, fwd, res_bwd)) return true;
      }
      else; // next candidate
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

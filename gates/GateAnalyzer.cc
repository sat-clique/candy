/*
 * GateAnalyzer.cc
 *
 *  Created on: Jul 21, 2016
 *      Author: markus
 */

#include "gates/GateAnalyzer.h"
#include "core/Solver.h"

using namespace std;
using namespace Glucose;

static const char* _cat = "GATE RECOGNITION";
static IntOption opt_gate_tries(_cat, "gate-tries", "Number of heuristic clause selections to enter recursion", 0, IntRange(0, INT32_MAX));
static BoolOption opt_patterns(_cat, "gate-patterns", "Enable Pattern-based Gate Detection", false);
static BoolOption opt_semantic(_cat, "gate-semantic", "Enable Semantic Gate Detection", false);
static BoolOption opt_holistic(_cat, "gate-holistic", "Enable Holistic Gate Detection", false);
static BoolOption opt_decompose(_cat, "gate-decompose", "Enable Local Blocked Decomposition", false);
static BoolOption opt_decompose_max_blocks(_cat, "gate-decompose-max-blocks", "Set Maximum Number of Blocks in Blocked Decomposition", 2);
static BoolOption opt_complete(_cat, "gate-complete", "Enable All The Options", false);

static const char* _debug = "DEBUGGING";
static BoolOption opt_verbose(_debug, "gate-debug", "Enable Debugging of Gate Analyzer", false);

GateAnalyzer::GateAnalyzer(CNFProblem& dimacs) :
	problem (dimacs),
	solver (),
	maxTries (opt_gate_tries),
	usePatterns (opt_patterns || opt_complete),
	useSemantic (opt_semantic || opt_holistic || opt_complete),
    useHolistic (opt_holistic || opt_complete),
    useDecomposition (opt_decompose || opt_complete),
    decompMaxBlocks (opt_decompose_max_blocks),
    verbose (opt_verbose) {
  gates = new vector<Cl*>(problem.nVars());
  gatesComplete = new vector<Gate>(problem.nVars());
  inputs.resize(2 * problem.nVars(), false);
  index = buildIndexFromClauses(problem.getProblem());
  if (useHolistic) solver.insertClauses(problem);
}

// heuristically select clauses
Lit GateAnalyzer::getRarestLiteral(vector<For>& index) {
  Lit min; min.x = INT_MAX;
  for (size_t l = 0; l < index.size(); l++) {
    if (index[l].size() > 0 && (min.x == INT_MAX || index[l].size() < index[min.x].size())) {
      min.x = l;
    }
  }
  assert(min.x < INT_MAX);
  return min;
}

bool GateAnalyzer::semanticCheck(vector<Cl*>& fwd, vector<Cl*>& bwd, Var o) {
  CNFProblem constraint;
  Lit alit = mkLit(problem.nVars(), false);
  Cl clause;
  for (const For& f : { fwd, bwd })
  for (Cl* cl : f) {
    clause.push_back(alit);
    for (Lit l : *cl) {
      if (var(l) != o) {
        clause.push_back(l);
      }
    }
    constraint.readClause(clause);
    clause.clear();
  }
  solver.insertClauses(constraint);
  bool isRightUnique = !solver.solve(~alit);
  solver.addClause(alit);
  return isRightUnique;
}

// clause patterns of full encoding
bool GateAnalyzer::completePattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs) {
  // precondition: fwd and bwd constrain exactly the same inputs (in opposite polarity)
  // and fwd blocks bwd on the output literal
  // given a total of 2^n blocked clauses implies that we have no redundancy in the n inputs
  set<Var> vars;
  for (Lit l : inputs) vars.insert(var(l));
  return fwd.size() == bwd.size() && 2*fwd.size() == pow(2, vars.size()) && 2*vars.size() == inputs.size();
}

// clause patterns of full encoding
bool GateAnalyzer::fullPattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs) {
  // precondition: fwd and bwd constrain exactly the same inputs (in opposite polarity)
  // and fwd blocks bwd on the output literal
  set<Var> vars;
  for (Lit l : inputs) vars.insert(var(l));
  bool fullOr = fwd.size() == 1 && fixedClauseSize(bwd, 2);
  bool fullAnd = bwd.size() == 1 && fixedClauseSize(fwd, 2);
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
    if (f.size() > 0 && (isBlocked(o, f, g) || useDecomposition && isBlockedAfterVE(o, f, g))) {
      bool mono = !inputs[o] || !inputs[~o];
      set<Lit> s, t;
      for (Cl* c : f) for (Lit l : *c) if (l != ~o) s.insert(l);
      if (!mono) for (Cl* c : g) for (Lit l : *c) if (l != o) t.insert(~l);
      bool gate = mono || (usePatterns && s == t && fullPattern(f, g, s)) || (useSemantic && semanticCheck(f, g, var(o)));
      if (gate) {
        nGates++;
        (*gates)[var(o)] = new Cl(s.begin(), s.end());
        literals.insert(literals.end(), s.begin(), s.end());
        for (Lit l : s) {
          inputs[l]++;
          if (!mono) inputs[~l]++;
        }
        //###
        (*gatesComplete)[var(o)].out = o;
        (*gatesComplete)[var(o)].notMono = !mono;
        (*gatesComplete)[var(o)].fwd = new For();
        (*gatesComplete)[var(o)].fwd->insert((*gatesComplete)[var(o)].fwd->end(), f.begin(), f.end());
        (*gatesComplete)[var(o)].bwd = new For();
        (*gatesComplete)[var(o)].bwd->insert((*gatesComplete)[var(o)].bwd->end(), g.begin(), g.end());
        (*gatesComplete)[var(o)].inp = new vector<Lit>();
        (*gatesComplete)[var(o)].inp->insert((*gatesComplete)[var(o)].inp->end(), s.begin(), s.end());
        //###
        removeFromIndex(index, f);
        removeFromIndex(index, g);
      }
    }
  }
}

void GateAnalyzer::analyze() {
  set<Lit> next;

  // start recognition with unit literals
  for (Cl* c : problem.getProblem()) {
    if (c->size() == 1) {
      roots.push_back(c);
      removeFromIndex(index, c);
      next.insert((*c)[0]);
    }
  }

  analyze(next);

  // clause selection loop
  for (int k = 0; k < maxTries; k++) {
    next.clear();
    Lit lit = getRarestLiteral(index);
    vector<Cl*>& clauses = index[lit];
    for (Cl* c : clauses) {
      next.insert(c->begin(), c->end());
    }
    roots.insert(roots.end(), clauses.begin(), clauses.end());
    removeFromIndex(index, clauses);
    analyze(next);
  }
}



//######################
// work in progress:

// precondition: ~o \in f[i] and o \in g[j]
bool GateAnalyzer::isBlockedAfterVE(Lit o, For f, For g) {
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

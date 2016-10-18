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
static BoolOption opt_lookahead(_cat, "gate-lookahead", "Enable Lookahead for additional encoding vars", false);
static BoolOption opt_complete(_cat, "gate-complete", "Enable All The Options", false);

static const char* _debug = "DEBUGGING";
static BoolOption opt_verbose(_debug, "gate-debug", "Enable Debugging of Gate Analyzer", false);

GateAnalyzer::GateAnalyzer(CNFProblem& dimacs) :
	problem (dimacs),
	solver (),
	maxTries (opt_gate_tries),
	usePatterns (opt_patterns || opt_complete),
	useSemantic (opt_semantic || opt_complete),
    useHolistic (opt_holistic || opt_complete),
    useLookahead (opt_lookahead || opt_complete),
    verbose (opt_verbose) {
  gates = new For(problem.nVars());
  inputs.resize(2 * problem.nVars(), false);
  index.resize(2 * problem.nVars());
  for (Cl* c : problem.getProblem()) for (Lit l : *c) {
    index[l].push_back(c);
  }
  assert(checkIndexConsistency());
  if (useHolistic) solver.insertClauses(problem);
  assert(checkIndexConsistency());
}

// heuristically select clauses
Lit GateAnalyzer::getRarestLiteral(vector<For>& index) {
  unsigned int min = INT_MAX;
  for (size_t l = 0; l < index.size(); l++) {
    if (index[l].size() > 0 && (min == INT_MAX || index[l].size() < index[min].size())) {
      min = l;
    }
  }
  assert(min < INT_MAX);
  Lit minLit;
  minLit.x = min;
  return minLit;
}

bool GateAnalyzer::semanticCheck(vector<Cl*>& fwd, vector<Cl*>& bwd, Var o) {
  if (verbose) {
    printf("semantic check for output %i \n", o+1);
    printf("forward clauses: \n");
    printClauses(fwd);
    printf("backward clauses: \n");
    printClauses(bwd);
  }

  CNFProblem constraint;
  Lit alit = mkLit(problem.nVars(), false);
  Cl clause;
  set<Var> vars;
  for (const For& f : { fwd, bwd })
  for (Cl* cl : f) {
    clause.push_back(alit);
    for (Lit l : *cl) {
      if (var(l) != o) {
        clause.push_back(l);
        vars.insert(var(l));
      }
    }
    constraint.readClause(clause);
    clause.clear();
  }
  solver.insertClauses(constraint);
  bool isRightUnique = !solver.solve(~alit);
  if (!isRightUnique) {
    printf("###\n Right-uniqueness check failed. Model is: \n");
    for (Var v : vars)
      if (solver.model[v] != l_Undef)
        printf("%s%s%d", (v == 0) ? "" : " ", (solver.model[v] == l_True) ? "" : "-", v + 1);
    printf("\n ###\n ");
  }
  printf("+++\n");
  solver.printProblem();
  printf("+++\n");
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

//    if (verbose) {
      printf("Candidate Output is %s%i \n", sign(o)?"-":"", var(o)+1);
//    }

    For& f = index[~o], g = index[o];
    if (f.size() > 0 && (isBlocked(o, f, g) || useLookahead && isBlockedAfterVE(o, f, g))) {
      printf("Got blocked set with output %s%i \n", sign(o)?"-":"", var(o)+1);
      bool mono = !inputs[o] || !inputs[~o];
      set<Lit> s, t;
      for (Cl* c : f) for (Lit l : *c) if (l != ~o) s.insert(l);
      if (!mono) for (Cl* c : g) for (Lit l : *c) if (l != o) t.insert(~l);
//      bool gate = mono || (usePatterns && s == t && fullPattern(f, g, s)) || ((useSemantic || useHolistic) && semanticCheck(f, g, var(o)));
      bool patterns = (usePatterns && s == t && fullPattern(f, g, s));
      bool semantic = ((useSemantic || useHolistic) && semanticCheck(f, g, var(o)));
      if (mono) printf("Be mono\n");
      if (patterns) printf("Be known pattern\n");
      if (semantic) printf("Be semantically right-unique\n");
//      if (gate) {
      if (mono || patterns || semantic) {
        nGates++;
        (*gates)[var(o)] = new Cl(s.begin(), s.end());
        printf("Registered new gate with output %i and inputs ", var(o)+1);
        printClause(*(*gates)[var(o)]);
        printf("\n");
        literals.insert(literals.end(), s.begin(), s.end());
        for (Lit l : s) {
          inputs[l]++;
          if (!mono) inputs[~l]++;
        }
        removeFromIndex(index, f);
        removeFromIndex(index, g);
      }
    }
  }
}

void GateAnalyzer::analyze() {
  assert(checkIndexConsistency());

  // populate index (except for unit-clauses, they go to roots immediately)
  for (Cl* c : problem.getProblem()) if (c->size() == 1) {
    roots.push_back(c);
    removeFromIndex(index, c);
  }

  assert(checkIndexConsistency());

//  if (verbose) {
    printf("Got %zu roots: \n", roots.size());
    printClauses(roots);
//  }

  // start with unit clauses
  set<Lit> next;
  for (Cl* c : roots) for (Lit l : *c) next.insert(l);
  analyze(next);

  // clause selection loop
  for (int k = 0; k < maxTries; k++) {
    next.clear();
    Lit lit = getRarestLiteral(index);
    vector<Cl*>& clauses = index[lit];
    for (Cl* c : clauses) {
      next.insert(c->begin(), c->end());
    }
    removeFromIndex(index, clauses);
    analyze(next);
  }
}

// precondition: ~o \in f[i] and o \in g[j]
bool GateAnalyzer::isBlockedAfterVE(Lit o, For& f, For& g) {
//    printf("Entering VE \n");

//  if (debug_counter > 300) return false;
//  else debug_counter++;

  if (f.size() > 16) {
    printf("too many clauses (n=%zu)\n", f.size());
    return false;
  }

  assert(checkIndexConsistency());
//    countIndex();

  // generate set of non-tautological resolvents
  For resolvents;
  for (Cl* a : f) for (Cl* b : g) {
    Cl* res = new Cl();
    if (!isBlocked(o, *a, *b)) {
      res->insert(res->end(), a->begin(), a->end());
      res->insert(res->end(), b->begin(), b->end());
      res->erase(std::remove_if(res->begin(), res->end(), [o](Lit l) { return var(l) == var(o); }), res->end());
      resolvents.push_back(res);
//        printf("A: ");
//        printClause(*a);
//        printf("B: ");
//        printClause(*b);
//        printf("A ° B: ");
//        printClause(*res);
    }

    // no candidate output
    if (resolvents.size() > (f.size()) * (g.size()) / 2) {
      printf("too many non-tautological resolvents (%zu > %zu * %zu / 2)\n", resolvents.size(), f.size(), g.size());
      for (Cl* res : resolvents) {
        delete res;
      }
      return false;
    }
  }

  // no non-tautological resolvents
  if (resolvents.empty()) return true;

  /***/
  printf("Found %zu non-tautological resolvents of %zu forward- and %zu backward clauses while analyzing lit %s%i: \n",
      resolvents.size(), f.size(), g.size(), sign(o)?"-":"", var(o)+1);
  for (Cl* r : resolvents) {
    printClause(*r);
  }
  /***/

  // generate set of input variables of candidate gate G = f and g
  set<Var> inputs;
  for (Cl* c : f) for (Lit l : *c) if (var(l) != var(o)) inputs.insert(var(l));
  for (Cl* c : g) for (Lit l : *c) if (var(l) != var(o)) inputs.insert(var(l));

  printf("%zu inputs: ", inputs.size());
  for (Var v : inputs) {
    printf("%i ", v+1);
  }
  printf("\n");

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
  if (candidate_vars.empty()) {
    for (Cl* res : resolvents) {
      delete res;
    }
    return false;
  }


  printf("%zu candidate vars: ", candidate_vars.size());
  for (Var v : candidate_vars) {
    printf("%i ", v+1);
  }
  printf("\n");

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

    printf("Candidate variable is %i \n", cand+1);

    bool pos_is_candidate = find(candidate_lits.begin(), candidate_lits.end(), output) != candidate_lits.end();
    bool neg_is_candidate = find(candidate_lits.begin(), candidate_lits.end(), ~output) != candidate_lits.end();
    // make sure fwd-clauses are usable in monotonic case
    if (!pos_is_candidate) output = ~output;

    // generate candidate definition for output
    For fwd, bwd;
    int nRejected = 0;

    for (Cl* c : index[~output]) {
      // clauses of candidate gate are still part of index (filter them out)
      if (find(f.begin(), f.end(), c) != f.end()) continue;
      if (find(g.begin(), g.end(), c) != g.end()) continue;
      // use clauses that constrain the inputs of our candidate gate only
      bool is_subset = true;
      for (Lit l : *c) if (l != ~output) {
        if (find(inputs.begin(), inputs.end(), var(l)) == inputs.end()) {
          is_subset = false;
          break;
        }
      }
      if (is_subset) fwd.push_back(c);
      else {
        printf("Rejected ");
        printClause(*c);
        nRejected++;
      }
    }

    for (Cl* c : index[output]) {
      // clauses of candidate gate are still part of index (filter them out)
      if (find(f.begin(), f.end(), c) != f.end()) continue;
      if (find(g.begin(), g.end(), c) != g.end()) continue;
      // use clauses that constrain the inputs of our candidate gate only
      bool is_subset = true;
      for (Lit l : *c) if (l != output) {
        if (find(inputs.begin(), inputs.end(), var(l)) == inputs.end()) {
          is_subset = false;
          break;
        }
      }
      if (is_subset) bwd.push_back(c);
      else {
        printf("Rejected ");
        printClause(*c);
        nRejected++;
      }
    }

    printf("Got filtered clause set: \n");
    printClauses(fwd);
    printClauses(bwd);

    if (fwd.empty()) {
      printf("fwd empty\n");
      continue;
    }

    // if candidate definition is functional
    // (check blocked state, in non-monotonic case also right-uniqueness <- use semantic holistic approach)
    bool functional = false;
    if (isBlocked(output, fwd, bwd)) {
      printf("Candidate set is blocked \\o/ \n");
      // output is used monotonic, iff
      // 1. it is pure in the candidate gate-definition
      // 2. it is pure in the current partial gate-structure
      // 3. it is pure in the remaining formula
      bool monotonic = !(pos_is_candidate && neg_is_candidate) && !this->inputs[toInt(output)] && bwd.size() == index[output].size();
      if (monotonic) {
        if (functional) {
          printf("Right-uniqueness not necessary as left-total relation is nested monotonic\n");
        }
        functional = true;
      } else {
        printf("Running semantic check \n");
//        CNFProblem prob;
//        prob.readClauses(f);
//        prob.readClauses(g);
//        solver.insertClauses(prob);
        For fwdsem, bwdsem;
        fwdsem.insert(fwdsem.end(), f.begin(), f.end());
        fwdsem.insert(fwdsem.end(), fwd.begin(), fwd.end());
        bwdsem.insert(bwdsem.end(), g.begin(), g.end());
        bwdsem.insert(bwdsem.end(), bwd.begin(), bwd.end());
        functional = semanticCheck(fwdsem, bwdsem, var(output));
        solver.printProblem();
        if (functional) {
          printf("Semantic check for right-uniqueness was successful\n");
        }
      }
    }

    // then local resolution is enough and then check resolve and check for gate-property again
    if (functional) {
      printf("Eliminate variable \n");
      // split resolvents
      For res_fwd, res_bwd;
      for (Cl* res : resolvents) {
        if (find(res->begin(), res->end(), ~output) == res->end()) {
          res_fwd.push_back(res);
        } else {
          res_bwd.push_back(res);
        }
      }
      if (isBlocked(~output, res_fwd, bwd) && isBlocked(~output, fwd, res_bwd)) {
        printf("All resolvents are tautologic after functional resolution\n");
        if (nRejected == 0) {
          removeFromIndex(index, fwd);
          removeFromIndex(index, bwd);
        }
        return true;
      }
    }
    else; // next candidate
  }

  for (Cl* res : resolvents) {
    delete res;
  }

  return false;
}

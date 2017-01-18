/*
 * GateAnalyzer.cc
 *
 *  Created on: Jul 21, 2016
 *      Author: markus
 */

#include "gates/GateAnalyzer.h"
#include "core/Solver.h"

namespace Candy {

GateAnalyzer::GateAnalyzer(CNFProblem& dimacs, int tries, bool patterns, bool semantic, bool holistic, bool lookahead, bool intensify) :
    problem (dimacs), solver (),
    maxTries (tries), usePatterns (patterns), useSemantic (semantic),
    useHolistic (holistic), useLookahead (lookahead), useIntensification (intensify)
{
  gates = new vector<Gate>(problem.nVars());
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
  //assert(min.x < INT_MAX);
  return min;
}

bool GateAnalyzer::semanticCheck(Var o, For& fwd, For& bwd) {
  CNFProblem constraint;
  Lit alit = Glucose::mkLit(problem.nVars(), false);
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
bool GateAnalyzer::patternCheck(Lit o, For& fwd, For& bwd, set<Lit>& inp) {
  // precondition: fwd blocks bwd on the o

  // check if fwd and bwd constrain exactly the same inputs (in opposite polarity)
  set<Lit> t;
  for (Cl* c : bwd) for (Lit l : *c) if (l != o) t.insert(~l);
  if (inp != t) return false;

  bool fullOr = fwd.size() == 1 && fixedClauseSize(bwd, 2);
  bool fullAnd = bwd.size() == 1 && fixedClauseSize(fwd, 2);
  if (fullOr || fullAnd) return true;

  // given a total of 2^n blocked clauses if size n+1 with n times the same variable should imply that we have no redundancy in the n inputs
  if (fwd.size() == bwd.size() && 2*fwd.size() == pow(2, inp.size()/2)) {
    set<Var> vars;
    for (Lit l : inp) vars.insert(var(l));
    return 2*vars.size() == inp.size();
  }

  return false;
}

// main analysis routine
vector<Lit> GateAnalyzer::analyze(vector<Lit>& roots, bool pat, bool sem, bool lah) {
  vector<Lit> frontier, remainder;

  for (Lit o : roots) {
    For& f = index[~o], g = index[o];
    if (f.size() > 0 && (isBlocked(o, f, g) || (lah && isBlockedAfterVE(o, f, g)))) {
      bool mono = !inputs[o] || !inputs[~o];
      set<Lit> inp;
      for (Cl* c : f) for (Lit l : *c) if (l != ~o) inp.insert(l);
      bool gate = mono || (pat && patternCheck(o, f, g, inp)) || (sem && semanticCheck(var(o), f, g));
      if (gate) {
        nGates++;
        frontier.insert(frontier.end(), inp.begin(), inp.end());
        for (Lit l : inp) {
          inputs[l]++;
          if (!mono) inputs[~l]++;
        }
        //###
        (*gates)[var(o)].out = o;
        (*gates)[var(o)].notMono = !mono;
        (*gates)[var(o)].fwd.insert((*gates)[var(o)].fwd.end(), f.begin(), f.end());
        (*gates)[var(o)].bwd.insert((*gates)[var(o)].bwd.end(), g.begin(), g.end());
        (*gates)[var(o)].inp.insert((*gates)[var(o)].inp.end(), inp.begin(), inp.end());
        //###
        removeFromIndex(index, f);
        removeFromIndex(index, g);
      }
      else {
        remainder.push_back(o);
      }
    }
    else {
      remainder.push_back(o);
    }
  }

  roots.swap(remainder);

  return frontier;
}

void GateAnalyzer::analyze() {
  vector<Lit> next;

  // start recognition with unit literals
  for (Cl* c : problem.getProblem()) {
    if (c->size() == 1) {
      roots.push_back(c);
      removeFromIndex(index, c);
      next.insert(next.end(), (*c)[0]);
      inputs[(*c)[0]]++;
    }
  }

  if (useIntensification) {
    vector<Lit> remainder;
    bool patterns, semantic, lookahead, restart = false;
    for (int level = 0; level < 3; restart ? level = 0 : level++) {
      printf("Remainder size: %zu, Intensification level: %i\n", remainder.size(), level);

      restart = false;

      switch (level) {
      case 0: patterns = true; semantic = false; lookahead = false; break;
      case 1: patterns = false; semantic = true; lookahead = false; break;
      case 2: patterns = false; semantic = true; lookahead = true; break;
      default: assert(level >= 0 && level < 3); break;
      }

      if (!usePatterns && patterns) continue;
      if (!useSemantic && semantic) continue;
      if (!useLookahead && lookahead) continue;

      next.insert(next.end(), remainder.begin(), remainder.end());
      remainder.clear();

      while (next.size()) {
        vector<Lit> frontier = analyze(next, patterns, semantic, lookahead);
        if (level > 0 && frontier.size() > 0) restart = true;
        remainder.insert(remainder.end(), next.begin(), next.end());
        next.swap(frontier);
      }

      sort(remainder.begin(), remainder.end());
      remainder.erase(unique(remainder.begin(), remainder.end()), remainder.end());
      reverse(remainder.begin(), remainder.end());
    }
  }
  else {
    while (next.size()) {
      vector<Lit> frontier = analyze(next, usePatterns, useSemantic, useLookahead);
      next.swap(frontier);
    }
  }

  // clause selection loop
  for (int k = 0; k < maxTries; k++) {
    next.clear();
    Lit lit = getRarestLiteral(index);
    if (lit.x == INT_MAX) break; // index is empty
    vector<Cl*>& clauses = index[lit];
    for (Cl* c : clauses) {
      next.insert(next.end(), c->begin(), c->end());
      for (Lit l : *c) inputs[l]++;
    }
    roots.insert(roots.end(), clauses.begin(), clauses.end());
    removeFromIndex(index, clauses);
    while (next.size()) {
      vector<Lit> frontier = analyze(next, usePatterns, useSemantic, useLookahead);
      next.swap(frontier);
    }
  }
}



//######################
// work in progress:

// precondition: ~o \in f[i] and o \in g[j]
bool GateAnalyzer::isBlockedAfterVE(Lit o, For& f, For& g) {
  // generate set of non-tautological resolvents
  For resolvents;
  for (Cl* a : f) for (Cl* b : g) {
    if (!isBlocked(o, *a, *b)) {
      Cl* res = new Cl();
      res->insert(res->end(), a->begin(), a->end());
      res->insert(res->end(), b->begin(), b->end());
      res->erase(std::remove_if(res->begin(), res->end(), [o](Lit l) { return var(l) == var(o); }), res->end());
      resolvents.push_back(res);
    }
    if (resolvents.size() > 10) return false;
  }
  if (resolvents.empty()) return true; // the set is trivially blocked

#ifdef GADebug
  printf("Found %zu non-tautologic resolvents\n", resolvents.size());
#endif

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

#ifdef GADebug
  printf("Found %zu candidate literals for functional elimination: ", candidates.size());
  for (Lit lit : candidates) printf("%s%i ", sign(lit)?"-":"", var(lit)+1);
  printf("\n");
#endif

  // generate set of input variables of candidate gate (o, f, g)
  set<Var> inputs;
  for (Cl* c : f) for (Lit l : *c) if (var(l) != var(o)) inputs.insert(var(l));
  for (Cl* c : g) for (Lit l : *c) if (var(l) != var(o)) inputs.insert(var(l));

  vector<int> occCount(problem.nVars()*2);
  for (Lit cand : candidates) {
    for (For formula : { f, g }) for (Cl* clause : formula) {
      if (find(clause->begin(), clause->end(), cand) != clause->end()) {
        occCount[cand]++;
      }
    }
  }

  vector<Lit> sCand(candidates.begin(), candidates.end());
  sort(sCand.begin(), sCand.end(), [occCount](Lit a, Lit b) { return occCount[a] < occCount[b]; });

  for (Lit cand : sCand) {
    // generate candidate definition for output
    For fwd, bwd;

#ifdef GADebug
    printf("candidate literal: %s%i\n", sign(cand)?"-":"", var(cand)+1);
#endif

    for (Lit lit : { cand, ~cand })
    for (Cl* c : index[lit]) {
      // clauses of candidate gate (o, f, g) are still part of index (skip them)
      if (find(f.begin(), f.end(), c) != f.end()) continue;
      if (find(g.begin(), g.end(), c) != g.end()) continue;

      // use clauses that constrain the inputs of our candidate gate only
      bool is_subset = true;
      for (Lit l : *c) {
        if (find(inputs.begin(), inputs.end(), var(l)) == inputs.end()) {
          is_subset = false;
          break;
        }
      }
      if (is_subset) {
        if (lit == ~cand) fwd.push_back(c);
        else bwd.push_back(c);
      }
    }

    if (fwd.empty()) continue;

#ifdef GADebug
    printf("Found %zu clauses for candidate literal %s%i\n", fwd.size() + bwd.size(), sign(cand)?"-":"", var(cand)+1);
#endif

    // if candidate definition is functional
    // (check blocked state, in non-monotonic case also right-uniqueness <- use semantic holistic approach)
    bool monotonic = false;
    bool functional = false;
    if (isBlocked(cand, fwd, bwd)) {
      // output is used monotonic, iff
      // 1. it is pure in the already decoded partial gate-structure (use input array)
      bool pure1 = !this->inputs[cand];
      // 2. it is pure in f of the candidate gate-definition (o, f, g)
      bool pure2 = false;
      // 3. it is pure in the remaining formula (look at literals in index, bwd entails entire index)
      bool pure3 = false;
      monotonic = pure1 && pure2 && pure3;
      if (!monotonic) {
        functional = semanticCheck(var(cand), fwd, bwd);
      }

      // then local resolution is enough and then check resolve and check for gate-property again
      if (monotonic || functional) {
        // split resolvents
        For res_fwd, res_bwd;
        for (Cl* res : resolvents) {
          if (find(res->begin(), res->end(), ~cand) == res->end()) {
            res_fwd.push_back(res);
          } else {
            res_bwd.push_back(res);
          }
        }
        if (isBlocked(~cand, res_fwd, bwd) && isBlocked(~cand, fwd, res_bwd)) {
#ifdef GADebug
          printf("Blocked elimination found for candidate literal %s%i\n", sign(cand)?"-":"", var(cand)+1);
#endif
          return true;
        }
      }
      else; // next candidate
    }
  }

  return false;
}

}

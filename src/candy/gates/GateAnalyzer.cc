/* Copyright (c) 2016 Markus Iser

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.

 */

#include "candy/gates/GateAnalyzer.h"
#include "candy/core/Solver.h"
#include "candy/utils/CNFProblem.h"
#include "candy/utils/MemUtils.h"

namespace Candy {

GateAnalyzer::GateAnalyzer(CNFProblem& dimacs, int tries, bool patterns, bool semantic, bool holistic,
        bool lookahead, bool intensify, int lookahead_threshold, unsigned int conflict_budget,
        std::chrono::milliseconds timeout) :
            problem (dimacs), solver (backported_std::make_unique<DefaultSolver>()),
            maxTries (tries), usePatterns (patterns), useSemantic (semantic || holistic),
            useHolistic (holistic), useLookahead (lookahead), useIntensification (intensify),
            lookaheadThreshold(lookahead_threshold), semanticConflictBudget(conflict_budget), runtime(timeout)
{
    runtime.start();
    gates.resize(problem.nVars());
    inputs.resize(2 * problem.nVars(), false);
    index = buildIndexFromClauses(problem.getProblem());
    if (useHolistic) solver->addClauses(problem);
    solver->setIncrementalMode();
    solver->initNbInitialVars(problem.nVars());
    runtime.stop();
}

GateAnalyzer::~GateAnalyzer() {
}

// heuristically select clauses
Var GateAnalyzer::getRarestVariable(vector<For>& index) {
    int min_count = INT_MAX;
    Var min = -1;
    for (Var v = 0; v < problem.nVars(); v++) {
        Lit lit1 = mkLit(v, false);
        Lit lit2 = mkLit(v, true);
        int count = index[lit1].size() + index[lit2].size();
        if (count > 0 && count <= min_count) {
            min = v;
            min_count = count;
        }
    }
    //assert(min.x < INT_MAX);
    return min;
}

Lit GateAnalyzer::getRarestLiteral(vector<For>& index) {
    Lit min; min.x = INT_MAX;
    for (size_t l = 0; l < index.size(); l++) {
        if (index[l].size() > 0 && (min.x == INT_MAX || index[l].size() < index[min.x].size())) {
            min.x = l;
        }
    }
    return min;
}

bool GateAnalyzer::semanticCheck(Var o, For& fwd, For& bwd) {
    CNFProblem constraint;
    Lit alit = mkLit(problem.nVars()+assumptions.size(), false);
    assumptions.push_back(~alit);
    Cl clause;
    for (const For& f : { fwd, bwd })
        for (Cl* cl : f) {
            for (Lit l : *cl) {
                if (var(l) != o) {
                    clause.push_back(l);
                }
            }
            clause.push_back(alit);
            constraint.readClause(clause);
#ifdef GADebug
            printClause(&clause, true);
#endif
            clause.clear();
        }
    solver->addClauses(constraint);
    solver->setConfBudget(semanticConflictBudget);
    bool isRightUnique = solver->solve(assumptions) == l_False;
    assumptions.back() = alit;
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
vector<Lit> GateAnalyzer::analyze(vector<Lit>& candidates, bool pat, bool sem, bool lah) {
    vector<Lit> frontier, remainder;

    for (Lit o : candidates) {
        For& f = index[~o], g = index[o];
        if (f.size() > 0 && (isBlocked(o, f, g) || (lah && isBlockedAfterVE(o, f, g)))) {
            bool mono = false, pattern = false, semantic = false;
            set<Lit> inp;
            for (Cl* c : f) for (Lit l : *c) if (l != ~o) inp.insert(l);
            mono = inputs[o] == 0 || inputs[~o] == 0;
            if (!mono) pattern = pat && patternCheck(o, f, g, inp);
            if (!mono && !pattern) semantic = sem && semanticCheck(var(o), f, g);
#ifdef GADebug
            if (mono) printf("Candidate output %s%i is nested monotonically\n", sign(o)?"-":"", var(o)+1);
            if (pattern) printf("Candidate output %s%i matches pattern\n", sign(o)?"-":"", var(o)+1);
            if (semantic) printf("Candidate output %s%i passed semantic test\n", sign(o)?"-":"", var(o)+1);
#endif
            if (mono || pattern || semantic) {
                nGates++;
                frontier.insert(frontier.end(), inp.begin(), inp.end());
                for (Lit l : inp) {
                    inputs[l]++;
                    if (!mono) inputs[~l]++;
                }
                //###
                gates[var(o)].out = o;
                gates[var(o)].notMono = !mono;
                gates[var(o)].fwd.insert(gates[var(o)].fwd.end(), f.begin(), f.end());
                gates[var(o)].bwd.insert(gates[var(o)].bwd.end(), g.begin(), g.end());
                gates[var(o)].inp.insert(gates[var(o)].inp.end(), inp.begin(), inp.end());
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

    candidates.swap(remainder);

    return frontier;
}


void GateAnalyzer::analyze(vector<Lit>& candidates) {
    if (useIntensification) {
        vector<Lit> remainder;
        bool patterns = false, semantic = false, lookahead = false, restart = false;
        for (int level = 0; level < 3 && !runtime.hasTimeout(); restart ? level = 0 : level++) {
#ifdef GADebug
            printf("Remainder size: %zu, Intensification level: %i\n", remainder.size(), level);
#endif

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

            candidates.insert(candidates.end(), remainder.begin(), remainder.end());
            remainder.clear();

            while (candidates.size() && !runtime.hasTimeout()) {
                vector<Lit> frontier = analyze(candidates, patterns, semantic, lookahead);
                if (level > 0 && frontier.size() > 0) restart = true;
                remainder.insert(remainder.end(), candidates.begin(), candidates.end());
                candidates.swap(frontier);
            }

            sort(remainder.begin(), remainder.end());
            remainder.erase(unique(remainder.begin(), remainder.end()), remainder.end());
            reverse(remainder.begin(), remainder.end());
        }
    }
    else {
        while (candidates.size() && !runtime.hasTimeout()) {
            vector<Lit> frontier = analyze(candidates, usePatterns, useSemantic, useLookahead);
            candidates.swap(frontier);
        }
    }
}

void GateAnalyzer::analyze() {
    vector<Lit> next;

    runtime.start();

    // start recognition with unit literals
    for (Cl* c : problem.getProblem()) {
        if (c->size() == 1) {
            roots.push_back(c);
            removeFromIndex(index, c);
            next.push_back((*c)[0]);
            inputs[(*c)[0]]++;
        }
    }

    analyze(next);

    // clause selection loop
    for (int k = 0; k < maxTries && !runtime.hasTimeout(); k++) {
        next.clear();
        Lit lit = getRarestLiteral(index);
        if (lit.x == INT_MAX) break; // index is empty
        vector<Cl*> clauses;
        clauses.insert(clauses.end(), index[lit].begin(), index[lit].end());
        index[lit].clear();
        removeFromIndex(index, clauses);
        roots.insert(roots.end(), clauses.begin(), clauses.end());
        for (Cl* c : clauses) {
            next.insert(next.end(), c->begin(), c->end());
            for (Lit l : *c) inputs[l]++;
        }
        analyze(next);
    }

    runtime.stop();
}

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
        if ((int)resolvents.size() > lookaheadThreshold) return false;
    }
    if (resolvents.empty()) return true; // the set is trivially blocked

#ifdef GADebug
    printf("Found %zu non-tautologic resolvents\n", resolvents.size());
#endif

    // generate set of literals whose variable occurs in every non-taut. resolvent (by successive intersection of resolvents)
    vector<Var> candidates;
    for (Lit l : *resolvents[0]) candidates.push_back(var(l));
    for (size_t i = 1; i < resolvents.size(); i++) {
        if (candidates.empty()) break;
        vector<Var> next_candidates;
        for (Lit lit : *resolvents[i]) {
            if (find(candidates.begin(), candidates.end(), var(lit)) != candidates.end()) {
                next_candidates.push_back(var(lit));
            }
        }
        std::swap(candidates, next_candidates);
        next_candidates.clear();
    }
    if (candidates.empty()) return false; // no candidate output

#ifdef GADebug
    printf("Found %zu candidate variables for functional elimination: ", candidates.size());
    for (Var v : candidates) printf("%i ", v+1);
    printf("\n");
#endif

    // generate set of input variables of candidate gate (o, f, g)
    vector<Var> inputs;
    vector<int> occCount(problem.nVars()+1);
    for (For formula : { f, g })  for (Cl* c : formula)  for (Lit l : *c) {
        if (var(l) != var(o)) {
            inputs.push_back(var(l));
            occCount[var(l)]++;
        }
    }

    sort(inputs.begin(), inputs.end());
    inputs.erase(unique(inputs.begin(), inputs.end()), inputs.end());

    sort(candidates.begin(), candidates.end(), [occCount](Var a, Var b) { return occCount[a] < occCount[b]; });

    for (Var cand : candidates) {
        // generate candidate definition for output
        For fwd, bwd;
        Lit out = mkLit(cand, false);

#ifdef GADebug
        printf("candidate variable: %i\n", cand+1);
#endif

        for (Lit lit : { out, ~out })
            for (Cl* c : index[lit]) {
                // clauses of candidate gate (o, f, g) are still part of index (skip them)
                if (find(f.begin(), f.end(), c) == f.end() && find(g.begin(), g.end(), c) == g.end()) {
                    // use clauses that constrain the inputs of our candidate gate only
                    bool is_subset = true;
                    for (Lit l : *c) {
                        if (find(inputs.begin(), inputs.end(), var(l)) == inputs.end()) {
                            is_subset = false;
                            break;
                        }
                    }
                    if (is_subset) {
                        if (lit == ~out) fwd.push_back(c);
                        else bwd.push_back(c);
                    }
                }
            }

#ifdef GADebug
        printf("Found %zu clauses for candidate variable %i\n", fwd.size() + bwd.size(), cand+1);
#endif

        // if candidate definition is functional
        if ((fwd.size() > 0 || bwd.size() > 0) && isBlocked(out, fwd, bwd) && semanticCheck(cand, fwd, bwd)) {
            // split resolvents by output literal 'out' of the function defined by 'fwd' and 'bwd'
            For res_fwd, res_bwd;
            for (Cl* res : resolvents) {
                if (find(res->begin(), res->end(), ~out) != res->end()) {
                    res_fwd.push_back(res);
                } else {
                    res_bwd.push_back(res);
                }
            }
            if ((res_fwd.size() == 0 || bwd.size() > 0) && (res_bwd.size() == 0 || fwd.size() > 0))
                if (isBlocked(out, res_fwd, bwd) && isBlocked(~out, res_bwd, fwd)) {
#ifdef GADebug
                    printf("Blocked elimination found for candidate variable %i\n", cand+1);
#endif
                    return true;
                }
        }
    }

    return false;
}

bool GateAnalyzer::hasTimeout() const {
    return runtime.hasTimeout();
}

}

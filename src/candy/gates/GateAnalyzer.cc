/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#include "candy/gates/GateAnalyzer.h"
#include "candy/gates/GateProblem.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/CNFProblem.h"
#include "candy/utils/MemUtils.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/frontend/CandyBuilder.h"

#include <iterator>
#include <vector>

namespace Candy {

GateAnalyzer::GateAnalyzer(const CNFProblem& dimacs, GateRecognitionMethod method, int tries, double timeout) :
            problem(dimacs), gate_problem(*new GateProblem { problem }), runtime(timeout), 
            maxTries (tries), usePatterns (false), useSemantic (false), useHolistic (false), useIntensification (false)
{
    index.resize(2 * problem.nVars());
    inputs.resize(2 * problem.nVars(), false);
    could_be_blocked.resize(problem.nVars(), true);

    // build index
    for (Cl* c : problem) for (Lit l : *c) {
        index[l].push_back(c);
    }

    switch (method) {
        case GateRecognitionMethod::Patterns: usePatterns = true; break;
        case GateRecognitionMethod::Semantic: useSemantic = true; break;
        case GateRecognitionMethod::Holistic: useHolistic = true; break;
        case GateRecognitionMethod::IntensifyPS: useIntensification = true; // fall-through:
        case GateRecognitionMethod::PatSem: usePatterns = true; useSemantic = true; break;
        case GateRecognitionMethod::IntensifyOSH: useIntensification = true; // fall-through:
        case GateRecognitionMethod::PatHol: usePatterns = true; useHolistic = true; break;
        default: usePatterns = true;
    }

    if (useSemantic || useHolistic) {
        SolverOptions::opt_sort_variables = false;
        SolverOptions::opt_sort_watches = false;
        SolverOptions::opt_preprocessing = false;
        solver = createSolver(); 
    }
}

GateAnalyzer::~GateAnalyzer() {}

std::vector<Lit> GateAnalyzer::getRarestLiterals(std::vector<For>& index) {
    std::vector<Lit> result;
    unsigned int min = UINT_MAX;
    for (Lit l = Lit(0, false); l.x < (int32_t)index.size(); l.x++) {
        if (index[l].size() > 0 && index[l].size() < min) {
            min = index[l].size();
            result.clear();
            result.push_back(l);
        }
        else if (index[l].size() == min) {
            result.push_back(l);
        }
    }
    return result;
}

std::vector<Cl> GateAnalyzer::getBestRoots() {
    std::vector<Cl> clauses;
    std::vector<Cl*> clausesp;
    std::vector<Lit> lits = getRarestLiterals(index);
    if (lits.empty()) return clauses;
    Lit best = lits.back();
    clausesp.insert(clausesp.end(), index[best].begin(), index[best].end());
    index[best].clear();
    removeFromIndex(index, clausesp);

    for (Cl* clause : clausesp) clauses.push_back(*clause);

    return clauses;
}

bool GateAnalyzer::semanticCheck(Var o, For& fwd, For& bwd) {
    CNFProblem constraint;
    Cl clause;
    for (const For& f : { fwd, bwd }) {
        for (Cl* cl : f) {
            for (Lit l : *cl) {
                if (l.var() != o) clause.push_back(l);
            }
            constraint.readClause(clause);
            clause.clear();
        }
    }
    if (useHolistic) {
        // include the resolution environments of each input
        std::vector<Cl*> resolution_environment;
        for (Cl* clause : constraint) {
            for (Lit lit : *clause) {
                resolution_environment.insert(resolution_environment.end(), index[lit].begin(), index[lit].end());
            }
        }
        std::sort(resolution_environment.begin(), resolution_environment.end());
        std::vector<Cl*>::iterator end = std::unique(resolution_environment.begin(), resolution_environment.end());
        for (auto it = resolution_environment.begin(); it != end; it++) {
            constraint.readClause(**it);
        }
    }
    solver->clear();
    constraint.normalizeVariableNames(); //<- crucial for performance
    solver->init(constraint);
    lbool result = solver->solve();
    return (result == l_False);
}


// clause patterns of full encoding
bool GateAnalyzer::patternCheck(Lit o, For& fwd, For& bwd, std::set<Lit>& inp) {
    // precondition: fwd blocks bwd on the o

    // check if fwd and bwd constrain exactly the same inputs (in opposite polarity)
    std::set<Lit> t;
    for (Cl* c : bwd) for (Lit l : *c) if (l != o) t.insert(~l);
    if (inp != t) return false;

    bool fullOr = fwd.size() == 1 && fixedClauseSize(bwd, 2);
    bool fullAnd = bwd.size() == 1 && fixedClauseSize(fwd, 2);
    if (fullOr || fullAnd) return true;

    // given a total of 2^n blocked clauses if size n+1 with n times the same variable should imply that we have no redundancy in the n inputs
    if (fwd.size() == bwd.size() && 2*fwd.size() == pow(2, inp.size()/2)) {
        std::set<Var> vars;
        for (Lit l : inp) vars.insert(l.var());
        return 2*vars.size() == inp.size();
    }

    return false;
}

// main analysis routine
std::vector<Lit> GateAnalyzer::analyze(std::vector<Lit>& candidates, bool pat, bool sem, bool lah) {
    std::vector<Lit> frontier, remainder;

    for (Lit o : candidates) {
        For& f = index[~o], g = index[o];
        if (!runtime.hasTimeout() && f.size() > 0 && isBlocked(o, f, g)) {
            bool mono = false, pattern = false, semantic = false;
            std::set<Lit> inp;
            for (Cl* c : f) for (Lit l : *c) if (l != ~o) inp.insert(l);
            mono = inputs[o] == 0 || inputs[~o] == 0;
            if (!mono) pattern = pat && patternCheck(o, f, g, inp);
            if (!mono && !pattern) {
                semantic = sem && semanticCheck(o.var(), f, g);
            }
#ifdef GADebug
            if (mono) printf("Candidate output %s%i is nested monotonically\n", o.sign()?"-":"", o.var()+1);
            if (pattern) printf("Candidate output %s%i matches pattern\n", o.sign()?"-":"", o.var()+1);
            if (semantic) printf("Candidate output %s%i passed semantic test\n", o.sign()?"-":"", o.var()+1);
#endif
            if (mono || pattern || semantic) {
                frontier.insert(frontier.end(), inp.begin(), inp.end());
                for (Lit l : inp) {
                    inputs[l]++;
                    if (!mono) inputs[~l]++;
                }
                gate_problem.addGate(o, f, g, inp, !mono); 
                gate_problem.addGateStats(pattern, semantic, semantic ? solver->getStatistics().nConflicts() : 0);
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

void GateAnalyzer::analyze(std::vector<Lit>& candidates) {
    if (useIntensification) {
        std::vector<Lit> remainder;
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
                std::vector<Lit> frontier = analyze(candidates, patterns, semantic, lookahead);
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
            std::vector<Lit> frontier = analyze(candidates, usePatterns, useSemantic, useLookahead);
            candidates.swap(frontier);
        }
    }
}

void GateAnalyzer::analyze() {
    std::vector<Lit> next;

    runtime.start();

    // start recognition with unit literals
    for (Cl* c : problem) {
        if (c->size() == 1) {
            gate_problem.roots.push_back(*c);
            removeFromIndex(index, c);
            next.push_back((*c)[0]);
            inputs[(*c)[0]]++;
        }
    }

    analyze(next);

    // clause selection loop
    for (int k = 0; k < maxTries && !runtime.hasTimeout(); k++) {
        std::vector<Cl> clauses = getBestRoots();
        gate_problem.roots.insert(gate_problem.roots.end(), clauses.begin(), clauses.end());
        for (Cl& c : clauses) {
            next.insert(next.end(), c.begin(), c.end());
            for (Lit l : c) inputs[l]++;
        }
        analyze(next);
    }

    runtime.stop();
}

bool GateAnalyzer::hasTimeout() const {
    return runtime.hasTimeout();
}

/**
  * Experimental: Tries to detect a common sub-gate in order to decode a gate
// precondition: ~o \in f[i] and o \in g[j]
bool GateAnalyzer::isBlockedAfterVE(Lit o, For& f, For& g) {
    // generate set of non-tautological resolvents
    
    std::vector<Cl> resolvents;
    for (Cl* a : f) for (Cl* b : g) {
        if (!isBlocked(o, *a, *b)) {
            resolvents.resize(resolvents.size() + 1); // new clause gets created at the back
            Cl& res = resolvents.back();
            res.insert(res.end(), a->begin(), a->end());
            res.insert(res.end(), b->begin(), b->end());
            res.erase(std::remove_if(res.begin(), res.end(), [o](Lit l) { return l.var() == o.var(); }), res.end());
        }
        if ((int)resolvents.size() > lookaheadThreshold) return false;
    }
    if (resolvents.empty()) return true; // the set is trivially blocked

#ifdef GADebug
    printf("Found %zu non-tautologic resolvents\n", resolvents.size());
#endif

    // generate set of literals whose variable occurs in every non-taut. resolvent (by successive intersection of resolvents)
    std::vector<Var> candidates;
    for (Lit l : resolvents[0]) candidates.push_back(l.var());
    for (size_t i = 1; i < resolvents.size(); ++i) {
        if (candidates.empty()) break;
        std::vector<Var> next_candidates;
        for (Lit lit : resolvents[i]) {
            if (find(candidates.begin(), candidates.end(), lit.var()) != candidates.end()) {
                next_candidates.push_back(lit.var());
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
    std::vector<Var> inputs;
    std::vector<int> occCount(problem.nVars()+1);
    for (For formula : { f, g })  for (Cl* c : formula)  for (Lit l : *c) {
        if (l.var() != o.var()) {
            inputs.push_back(l.var());
            occCount[l.var()]++;
        }
    }

    sort(inputs.begin(), inputs.end());
    inputs.erase(unique(inputs.begin(), inputs.end()), inputs.end());

    sort(candidates.begin(), candidates.end(), [occCount](Var a, Var b) { return occCount[a] < occCount[b]; });

    for (Var cand : candidates) {
        // generate candidate definition for output
        For fwd, bwd;
        Lit out = Lit(cand, false);

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
                        if (find(inputs.begin(), inputs.end(), l.var()) == inputs.end()) {
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
            for (Cl& res : resolvents) {
                if (find(res.begin(), res.end(), ~out) != res.end()) {
                    res_fwd.push_back(&res);
                } else {
                    res_bwd.push_back(&res);
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
*/

}

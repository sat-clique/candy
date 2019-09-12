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

GateAnalyzer::GateAnalyzer(const CNFProblem& problem_, GateRecognitionMethod method, ClauseSelectionMethod selection, int tries, double timeout) :
        problem(problem_), gate_problem(*new GateProblem { problem_ }), clause_selection_method(selection), runtime(timeout), 
        maxTries (tries), usePatterns (false), useSemantic (false), useHolistic (false), useIntensification (false)
{
    index.resize(2 * problem.nVars());
    could_be_blocked.resize(problem.nVars(), true);

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
        case GateRecognitionMethod::PatSemHol: usePatterns = true; useSemantic = true; useHolistic = true; break;
        default: usePatterns = true;
    }

    if (useSemantic || useHolistic) {
        SolverOptions::opt_sort_variables = false;
        SolverOptions::opt_sort_watches = false;
        SolverOptions::opt_preprocessing = false;
        solver = createSolver(); 
    }
}

GateAnalyzer::~GateAnalyzer() { }

/**
 * Clause Selection Heuristics
 * */
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

std::vector<Cl*> GateAnalyzer::getUnitClauses() {
    std::vector<Cl*> clauses;
    for (Cl* c : problem) {
        if (c->size() == 1) {
            clauses.push_back(c);
        }
    }
    return clauses;
}

std::vector<Cl*> GateAnalyzer::getClausesWithRareLiterals() {
    std::vector<Cl*> clauses;
    if (clauses.empty()) {
        std::vector<Lit> lits = getRarestLiterals(index);
        if (!lits.empty()) {
            Lit best = lits.back();
            clauses.insert(clauses.end(), index[best].begin(), index[best].end());
        }
    }
    return clauses;
}

std::vector<Cl*> GateAnalyzer::getClausesWithMaximalLiterals() {
    std::vector<Cl*> clauses;
    if (clauses.empty()) {
        std::vector<Lit> lits = getRarestLiterals(index);
        if (!lits.empty()) {
            Lit best = lits.back();
            clauses.insert(clauses.end(), index[best].begin(), index[best].end());
        }
    }
    return clauses;
}

/**
 * Entry Point for Gate Analysis.
 * 
 * Selects clauses using the configured clause-selection strategy 
 * and uses them as root-clauses in the subordinate analysis. 
 * */
void GateAnalyzer::analyze() {
    runtime.start();

    std::vector<Cl*> root_clauses = getUnitClauses();
    unsigned int count = 0;

    switch (clause_selection_method) {
        case ClauseSelectionMethod::UnitClausesThenMaximalLiterals: 
        case ClauseSelectionMethod::UnitClausesThenRareLiterals: 
            gate_recognition(root_clauses); 
            count++;
        default: break;
    }

    for (; count < maxTries && !runtime.hasTimeout(); count++) {
        switch (clause_selection_method) {
            case ClauseSelectionMethod::UnitClausesThenMaximalLiterals: 
            case ClauseSelectionMethod::MaximalLiterals: 
                root_clauses = getClausesWithMaximalLiterals();
                break;
            case ClauseSelectionMethod::UnitClausesThenRareLiterals: 
            case ClauseSelectionMethod::RareLiterals: 
                root_clauses = getClausesWithRareLiterals();
                break;
        }
        gate_recognition(root_clauses);
    }

    runtime.stop();
}

void GateAnalyzer::gate_recognition(std::vector<Cl*> roots) {
    std::vector<Lit> candidates;

    gate_problem.roots.insert(gate_problem.roots.end(), roots.begin(), roots.end());
    removeFromIndex(index, roots);

    for (Cl* clause : roots) {
        candidates.insert(candidates.end(), clause->begin(), clause->end());
        for (Lit l : *clause) gate_problem.setUsedAsInput(l);
    }

    if (useIntensification) {
        intensification_recognition(candidates);
    } else {
        classic_recognition(candidates);
    }
}

void GateAnalyzer::classic_recognition(std::vector<Lit> roots) {
    std::vector<Lit> candidates { roots.begin(), roots.end() };

    while (!candidates.empty()) {
        Lit candidate = candidates.back();
        candidates.pop_back();
        if (isGate(candidate, usePatterns, useSemantic, useHolistic)) { 
            candidates.insert(candidates.end(), gate_problem.getGate(candidate).inp.begin(), gate_problem.getGate(candidate).inp.end());
        }
    }
}

void GateAnalyzer::intensification_recognition(std::vector<Lit> roots) {
    std::vector<Lit> candidates;
    std::vector<Lit> remainder;
    std::vector<Lit> frontier { roots.begin(), roots.end() };

    for (int level = 0; level < 3; level++) {
        if (!usePatterns && level == 0 || !useSemantic && level == 1 || !useHolistic && level == 2) {
            continue; // skip disabled levels
        }

        if (frontier.empty()) { // add remainder for processing on the next level
            sort(remainder.begin(), remainder.end(), [](Lit l1, Lit l2) { return l1 > l2; });
            remainder.erase(std::unique(remainder.begin(), remainder.end()), remainder.end());
            candidates.swap(remainder);
        } else {
            candidates.swap(frontier);
        }

        for (Lit candidate : candidates) {
            if (isGate(candidate, level == 0, level == 2, level == 3)) { // try these with level 0 (pattern recognition) first
                frontier.insert(frontier.end(), gate_problem.getGate(candidate).inp.begin(), gate_problem.getGate(candidate).inp.end());
                level = -1; // restart analysis with pattern recognition on literals in frontier
            } else { 
                // try again with next level
                remainder.push_back(candidate);
            }
        }

        candidates.clear();
    }
}


/**
 * Main Gate Analysis Routine
 * 
 * Test if the remaining clauses in the index contain a gate definition 
 * with the given candidate literal as an output
 * 
 * Uses only the methods activated by the given flags
 * */
bool GateAnalyzer::isGate(Lit candidate, bool pat, bool sem, bool hol) {
    For& fwd = index[~candidate]; 
    For& bwd = index[candidate];
    if (fwd.size() > 0 && isBlocked(candidate, fwd, bwd)) {
        bool pattern = false, semantic = false, holistic = false;
        bool monotonous = gate_problem.isNestedMonotonous(candidate);
        
        if (!monotonous) {
            if (pat && patternCheck(candidate, fwd, bwd)) {
                pattern = true;
            }
            else if (sem && semanticCheck(candidate, fwd, bwd)) {
                semantic = true;
            }
            else if (hol && semanticCheck(candidate, fwd, bwd, true)) {
                holistic = true;
            }
        }

        if (monotonous || pattern || semantic || holistic) {
            gate_problem.addGate(candidate, fwd, bwd); 
            gate_problem.addGateStats(pattern, semantic, holistic, (semantic || holistic) ? solver->getStatistics().nConflicts() : 0);
            removeFromIndex(index, gate_problem.getGate(candidate).fwd);
            removeFromIndex(index, gate_problem.getGate(candidate).bwd);
            return true;
        }
        else {
            return false;
        }
    }
}


// clause patterns of full encoding
// precondition: fwd blocks bwd on the o
bool GateAnalyzer::patternCheck(Lit o, For& fwd, For& bwd) {
    // check if fwd and bwd constrain exactly the same inputs (in opposite polarity)
    std::set<Lit> fwd_inp, bwd_inp;
    for (Cl* c : fwd) for (Lit l : *c) if (l != ~o) fwd_inp.insert(l);
    for (Cl* c : bwd) for (Lit l : *c) if (l != o) bwd_inp.insert(~l);
    if (fwd_inp != bwd_inp) {
        return false;
    }
    // detect equivalence gates
    if (fwd.size() == 1 && bwd.size() == 1 && fwd.front()->size() == 2 && bwd.front()->size() == 2) {
        return true;
    }
    // detect or gates
    if (fwd.size() == 1 && fixedClauseSize(bwd, 2)) {
        return true;
    }
    // detect and gates
    if (bwd.size() == 1 && fixedClauseSize(fwd, 2)) {
        return true;
    }
    // given a total of 2^n blocked clauses if size n+1 with n times the same variable should imply that we have no redundancy in the n inputs
    if (fwd.size() == bwd.size() && 2*fwd.size() == pow(2, fwd_inp.size()/2)) {
        std::set<Var> vars;
        for (Lit l : fwd_inp) vars.insert(l.var());
        return 2*vars.size() == fwd_inp.size();
    }
    return false;
}

bool GateAnalyzer::semanticCheck(Lit o, For& fwd, For& bwd, bool holistic) {
    CNFProblem constraint;
    Cl clause;
    for (const For& f : { fwd, bwd }) {
        for (Cl* cl : f) {
            for (Lit l : *cl) {
                if (l.var() != o.var()) clause.push_back(l);
            }
            constraint.readClause(clause);
            clause.clear();
        }
    }
    if (holistic) {
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

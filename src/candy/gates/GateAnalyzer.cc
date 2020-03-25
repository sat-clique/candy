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
#include <unordered_set>

namespace Candy {

GateAnalyzer::GateAnalyzer(const CNFProblem& problem_, GateRecognitionMethod method, int tries, double timeout) :
        problem(problem_), gate_problem(*new GateProblem { problem_ }), runtime(timeout), index(problem_), 
        maxTries (tries), usePatterns (false), useSemantic (false), useHolistic (false), useIntensification (false)
{
    switch (method) {
        case GateRecognitionMethod::Patterns: usePatterns = true; break;
        case GateRecognitionMethod::Semantic: useSemantic = true; break;
        case GateRecognitionMethod::Holistic: useHolistic = true; break;
        case GateRecognitionMethod::PatSem: usePatterns = true; useSemantic = true; break;
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



std::vector<Cl*> GateAnalyzer::getUnitClauses() {
    std::vector<Cl*> clauses;
    for (Cl* c : problem) {
        if (c->size() == 1) {
            clauses.push_back(c);
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

    std::vector<Cl*> root_clauses;

    for (unsigned int count = 0; (maxTries == 0 || count < maxTries) && !runtime.hasTimeout(); count++) {
        if (count == 0) {
            root_clauses = getUnitClauses();
        }
        else {
            Lit lit = index.getMinimallyUnblockedLiteral();
            if (lit != lit_Undef) {
                root_clauses = index.getUnblockedClauses(lit);
            }
            else {
                break;
            }
        }

        if (root_clauses.empty()) continue;
        
        gate_recognition(root_clauses);
    }

    std::unordered_set<Cl*> remainder;
    for (size_t lit = 0; lit < index.size(); lit++) {
        //if (index[lit].size() > 0) std::cout << "Remainder " << index[lit] << std::endl;
        remainder.insert(index[lit].begin(), index[lit].end());
    }
    gate_problem.remainder.insert(gate_problem.remainder.end(), remainder.begin(), remainder.end());

    runtime.stop();
}

void GateAnalyzer::gate_recognition(std::vector<Cl*> roots) {
    std::vector<Lit> candidates;

    index.remove(roots);

    for (Cl* clause : roots) {
        gate_problem.roots.push_back(clause);
        candidates.insert(candidates.end(), clause->begin(), clause->end());
        for (Lit l : *clause) gate_problem.setUsedAsInput(l);
    }

    if (useIntensification) {
        recognition_with_intensification(candidates);
    } else {
        classic_recognition(candidates);
    }
}

void GateAnalyzer::classic_recognition(std::vector<Lit> roots) {
    std::vector<Lit> candidates;
    std::vector<Lit> frontier { roots.begin(), roots.end() };

    //std::cout << "Starting recogintion with the following roots: " << roots << std::endl;

    // while (!candidates.empty()) {
    //     Lit candidate = candidates.back();
    //     candidates.pop_back();
    while (!frontier.empty()) { // _breadth_ first search is important here (considering the symmetries in e.g. XOR-encodings)
        candidates.swap(frontier);

        for (Lit candidate : candidates) {
            //std::cout << "Candidate Literal is: " << candidate << std::endl;
            if (isGate(candidate, usePatterns, useSemantic, useHolistic)) { 
                //std::cout << "Found gate with inputs: " << gate_problem.getGate(candidate).inp << std::endl;
                frontier.insert(frontier.end(), gate_problem.getGate(candidate).inp.begin(), gate_problem.getGate(candidate).inp.end());
            }
        }
        candidates.clear();
    }
}

void GateAnalyzer::recognition_with_intensification(std::vector<Lit> roots) {
    std::vector<Lit> candidates;
    std::vector<Lit> frontier { roots.begin(), roots.end() };
    std::vector<Lit> remainder[3];

    for (int level = 0; level < (useHolistic ? 3 : 2); level++) {

        candidates.swap(frontier);
        for (Lit candidate : candidates) {
            if (isGate(candidate, level == 0, level == 1, level == 2)) { 
                // try these with level 0 (pattern recognition) first
                frontier.insert(frontier.end(), gate_problem.getGate(candidate).inp.begin(), gate_problem.getGate(candidate).inp.end());
            } 
            else { 
                // remember for next level
                remainder[level].push_back(candidate);
            }
        }
        candidates.clear();

        if (!frontier.empty()) {
            // restart analysis with pattern recognition
            level = -1; 
        } 
        else { 
            // use remainder for processing on the next level
            sort(remainder[level].begin(), remainder[level].end(), [](Lit l1, Lit l2) { return l1 > l2; });
            remainder[level].erase(std::unique(remainder[level].begin(), remainder[level].end()), remainder[level].end());
            frontier.swap(remainder[level]);
        }
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
    if (index[~candidate].size() > 0 && index.isBlockedSet(candidate)) {
        const For& fwd = index[~candidate];
        const For& bwd = index[candidate];
        bool pattern = false, semantic = false, holistic = false;
        bool monotonic = gate_problem.isNestedMonotonic(candidate);
        
        if (!monotonic) {
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

        if (monotonic || pattern || semantic || holistic) {
            gate_problem.addGate(candidate, fwd, bwd); 
            gate_problem.addGateStats(pattern, semantic, holistic, (semantic || holistic) ? solver->nConflicts() : 0);
            index.remove(gate_problem.getGate(candidate).fwd);
            index.remove(gate_problem.getGate(candidate).bwd);
            return true;
        }
        else if (sem || hol) {
            gate_problem.addUnsuccessfulStats(solver->nConflicts());
        }
    }
    return false;
}


// clause patterns of full encoding
// precondition: fwd blocks bwd on the o
bool GateAnalyzer::patternCheck(Lit o, const For& fwd, const For& bwd) {
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

bool GateAnalyzer::semanticCheck(Lit o, const For& fwd, const For& bwd, bool holistic) {
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

}

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

#ifndef GATE_ANALYZER
#define GATE_ANALYZER

#include <cstdlib>
#include <algorithm>
#include <memory>

#include <vector>
#include <set>

#include <climits>
#include <chrono>

#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"
#include "candy/gates/GateProblem.h"

#include "candy/utils/Runtime.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/frontend/CLIOptions.h"

namespace Candy {

enum class GateRecognitionMethod {
    Patterns = 0, // use gate patterns
    Semantic = 1, // use semantic checks
    Holistic = 2, // use semantic checks that include the resolution environment
    PatSem = 10, // use gate patterns and semantic checks 
    PatHol = 11, // use gate patterns and semantic checks that include the resolution environment
    IntensifyPS = 20, // use intensification strategy from patterns to semantic
    IntensifyOSH = 21// use intensification strategy from patterns to semantic to holistic
};

enum class ClauseSelectionMethod {
    UnitClausesThenMaximalLiterals = 0,
    UnitClausesThenRareLiterals = 1,
    MaximalLiterals = 2,
    RareLiterals = 3
};

class GateAnalyzer {

public:
    GateAnalyzer(const CNFProblem& problem, 
        GateRecognitionMethod method = static_cast<GateRecognitionMethod>(GateRecognitionOptions::method.get()), 
        ClauseSelectionMethod seleciton = static_cast<ClauseSelectionMethod>(GateRecognitionOptions::selection.get()), 
        int tries = GateRecognitionOptions::tries, 
        double timeout = GateRecognitionOptions::timeout);

    ~GateAnalyzer();

    GateProblem& getResult() const {
        return gate_problem;
    }

    GateProblem& getGateProblem() const {
        return gate_problem;
    }

    const CNFProblem& getCNFProblem() const {
        return problem;
    }

    bool hasTimeout() const {
        return runtime.hasTimeout();
    }

    const CNFProblem& problem;
    GateProblem& gate_problem;    
    ClauseSelectionMethod clause_selection_method;
    Runtime runtime;

    void analyze();

private:
    // problem to analyze:
    CandySolverInterface* solver;

    // control structures:
    std::vector<For> index; // occurrence lists
    std::vector<char> inputs; // flags to check if both polarities of literal are used as input (monotonicity)
    std::vector<char> could_be_blocked;

    // heuristic configuration:
    unsigned int maxTries = 1;
    bool usePatterns = false;
    bool useSemantic = false;
    bool useHolistic = false;
    bool useIntensification = false;

    // main analysis routines
    void analyze(std::vector<Cl*> roots);
    std::vector<Lit> analyze(std::vector<Lit>& candidates, bool pat, bool sem, bool dec);

    // clause selection heuristic
    std::vector<Lit> getRarestLiterals(std::vector<For>& index);
    std::vector<Cl*> getUnitClauses();
    std::vector<Cl*> getClausesWithRareLiterals();
    std::vector<Cl*> getClausesWithMaximalLiterals();

    // clause patterns of full encoding
    bool patternCheck(Lit o, For& fwd, For& bwd, std::set<Lit>& inputs);
    bool semanticCheck(Var o, For& fwd, For& bwd);

    // work in progress:
    bool isBlockedAfterVE(Lit o, For& f, For& g);

    // some helpers:
    bool isBlocked(Lit o, Cl& a, Cl& b) { // assert ~o \in a and o \in b
        for (Lit c : a) for (Lit d : b) if (c != ~o && c == ~d) return true;
        return false;
    }

    bool isBlocked(Lit o, For& f, For& g) { // assert ~o \in f[i] and o \in g[j]
        if (!could_be_blocked[o.var()]) return false;
        for (Cl* a : f) for (Cl* b : g) if (!isBlocked(o, *a, *b)) { could_be_blocked[o.var()] = false; return false; }
        return true;
    }

    bool isBlocked(Lit o, Cl* c, For& f) { // assert ~o \in c and o \in f[i]
        for (Cl* a : f) if (!isBlocked(o, *c, *a)) return false;
        return true;
    }

    bool fixedClauseSize(For& f, unsigned int n) {
        for (Cl* c : f) if (c->size() != n) return false;
        return true;
    }

    void removeFromIndex(std::vector<For>& index, Cl* clause) {
        for (Lit l : *clause) {
            For& h = index[l];
            h.erase(remove(h.begin(), h.end(), clause), h.end());
            could_be_blocked[l.var()] = true;
        }
    }

    void removeFromIndex(std::vector<For>& index, For& clauses) {
        For copy(clauses.begin(), clauses.end());
        for (Cl* c : copy) {
            removeFromIndex(index, c);
        }
    }

};

}
#endif
